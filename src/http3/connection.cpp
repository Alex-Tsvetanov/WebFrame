#include "coroute/http3/connection.hpp"

#ifdef COROUTE_HAS_HTTP3

#include <openssl/rand.h>

#include <array>
#include <cstring>

#include "coroute/http3/stateless.hpp"

// ngtcp2_callbacks and nghttp3_callbacks are large C structs that grow with every
// release, and only a handful of their members apply to a server. Naming each one
// just to zero it would be noise that has to be re-audited on every upgrade, and a
// designated initializer already zeroes what it does not mention. The warning is
// asking for the opposite of what keeps this correct.
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

namespace coroute::http3
{

	namespace
	{
		// The largest datagram this server will emit. Small enough to cross any path
		// without fragmenting, which matters because a fragmented QUIC packet is a
		// dropped QUIC packet on a good number of middleboxes.
		constexpr std::size_t max_datagram_size = 1452;

		// How many nghttp3 vectors to drain per packet. Past a handful the packet is
		// full anyway, so a larger array only costs stack.
		constexpr std::size_t max_write_vecs = 16;

		// ngtcp2 addresses are a (sockaddr*, len) pair. Endpoint is a fixed buffer
		// holding the same thing, so each conversion is a bounded copy.
		net::Endpoint endpoint_from(const ngtcp2_addr& addr) noexcept
		{
			net::Endpoint endpoint;
			const auto len = static_cast<std::size_t>(addr.addrlen);
			if (addr.addr != nullptr && len <= net::Endpoint::capacity)
			{
				std::memcpy(endpoint.bytes.data(), addr.addr, len);
				endpoint.len = static_cast<std::uint32_t>(len);
			}
			return endpoint;
		}

		void addr_from(ngtcp2_addr& out, const net::Endpoint& endpoint) noexcept
		{
			ngtcp2_addr_init(&out, reinterpret_cast<const sockaddr*>(endpoint.bytes.data()),
			                 static_cast<socklen_t>(endpoint.len));
		}

		Http3Connection* self_of(void* user_data) noexcept { return static_cast<Http3Connection*>(user_data); }

		Error quic_error(const char* what) { return Error::io(IoError::Unknown, what); }

		// ====================================================================
		// ngtcp2 callbacks
		// ====================================================================
		//
		// Every one is a C function pointer, so the object comes back through
		// user_data. The callbacks ngtcp2_crypto already provides are installed
		// directly rather than wrapped: wrapping would add a frame and a chance to get
		// the semantics wrong, and buy nothing.

		int cb_recv_stream_data(ngtcp2_conn*, std::uint32_t flags, std::int64_t stream_id, std::uint64_t,
		                        const std::uint8_t* data, std::size_t datalen, void* user_data, void*)
		{
			return self_of(user_data)->on_recv_stream_data(flags, stream_id, data, datalen);
		}

		int cb_stream_close(ngtcp2_conn*, std::uint32_t, std::int64_t stream_id, std::uint64_t rx_app_error_code,
		                    std::uint64_t, void* user_data, void*)
		{
			return self_of(user_data)->on_stream_close(stream_id, rx_app_error_code);
		}

		int cb_acked_stream_data_offset(ngtcp2_conn*, std::int64_t stream_id, std::uint64_t, std::uint64_t datalen,
		                                void* user_data, void*)
		{
			return self_of(user_data)->on_acked_stream_data(stream_id, datalen);
		}

		int cb_handshake_completed(ngtcp2_conn*, void* user_data)
		{
			return self_of(user_data)->on_handshake_completed();
		}

		int cb_get_new_connection_id(ngtcp2_conn*, ngtcp2_cid* cid, ngtcp2_stateless_reset_token* token,
		                             std::size_t cidlen, void* user_data)
		{
			return self_of(user_data)->on_new_connection_id(cid, token, cidlen);
		}

		void cb_rand(std::uint8_t* dest, std::size_t destlen, const ngtcp2_rand_ctx*)
		{
			if (RAND_bytes(dest, static_cast<int>(destlen)) != 1)
			{
				// ngtcp2 gives this callback no way to report failure. Zeroing is worse
				// than random but better than leaving the buffer uninitialised, and a
				// system whose CSPRNG has failed has larger problems than this packet.
				std::memset(dest, 0, destlen);
			}
		}

		int cb_stream_reset(ngtcp2_conn*, std::int64_t stream_id, std::uint64_t, std::uint64_t app_error_code,
		                    void* user_data, void*)
		{
			return self_of(user_data)->on_stream_close(stream_id, app_error_code);
		}

		// ====================================================================
		// nghttp3 callbacks
		// ====================================================================

		int cb_h3_recv_header(nghttp3_conn*, std::int64_t stream_id, std::int32_t, nghttp3_rcbuf* name,
		                      nghttp3_rcbuf* value, std::uint8_t, void* user_data, void*)
		{
			const nghttp3_vec name_buf = nghttp3_rcbuf_get_buf(name);
			const nghttp3_vec value_buf = nghttp3_rcbuf_get_buf(value);
			return self_of(user_data)->on_h3_recv_header(stream_id, {name_buf.base, name_buf.len},
			                                             {value_buf.base, value_buf.len});
		}

		int cb_h3_end_stream(nghttp3_conn*, std::int64_t stream_id, void* user_data, void*)
		{
			return self_of(user_data)->on_h3_end_stream(stream_id);
		}

		int cb_h3_stream_close(nghttp3_conn*, std::int64_t stream_id, std::uint64_t, void* user_data, void*)
		{
			return self_of(user_data)->on_h3_stream_close(stream_id);
		}

		int cb_h3_recv_data(nghttp3_conn*, std::int64_t stream_id, const std::uint8_t* data, std::size_t datalen,
		                    void* user_data, void*)
		{
			return self_of(user_data)->on_h3_recv_data(stream_id, {data, datalen});
		}

		int cb_h3_deferred_consume(nghttp3_conn*, std::int64_t, std::size_t, void*, void*) { return 0; }

		int cb_h3_begin_headers(nghttp3_conn*, std::int64_t, void*, void*) { return 0; }

		int cb_h3_stop_sending(nghttp3_conn*, std::int64_t, std::uint64_t, void*, void*) { return 0; }

		int cb_h3_reset_stream(nghttp3_conn*, std::int64_t, std::uint64_t, void*, void*) { return 0; }

		// Hands nghttp3 the response body. Called after submit_response has already
		// returned, which is why the body has to be owned by the Stream rather than by
		// whatever produced it.
		nghttp3_ssize cb_h3_read_data(nghttp3_conn*, std::int64_t, nghttp3_vec* vec, std::size_t veccnt,
		                              std::uint32_t* pflags, void*, void* stream_user_data)
		{
			auto* body = static_cast<const std::string*>(stream_user_data);
			*pflags = NGHTTP3_DATA_FLAG_EOF;
			if (body == nullptr || body->empty() || veccnt == 0)
			{
				return 0;
			}
			vec[0].base = reinterpret_cast<std::uint8_t*>(const_cast<char*>(body->data()));
			vec[0].len = body->size();
			return 1;
		}

	}  // namespace

	ngtcp2_tstamp now_ts() noexcept
	{
		const auto since = std::chrono::steady_clock::now().time_since_epoch();
		return static_cast<ngtcp2_tstamp>(std::chrono::duration_cast<std::chrono::nanoseconds>(since).count());
	}

	// ========================================================================
	// Construction
	// ========================================================================

	Http3Connection::Http3Connection(net::DatagramSocket& socket, std::size_t worker_index,
	                                 RequestHandler handler) noexcept
	    : socket_(socket), worker_index_(worker_index), handler_(std::move(handler))
	{
	}

	Http3Connection::~Http3Connection()
	{
		if (h3_ != nullptr)
		{
			nghttp3_conn_del(h3_);
		}
		if (conn_ != nullptr)
		{
			ngtcp2_conn_del(conn_);
		}
		// The ossl context wraps the SSL object, so it goes first.
		if (ossl_ctx_ != nullptr)
		{
			ngtcp2_crypto_ossl_ctx_del(ossl_ctx_);
		}
		if (ssl_ != nullptr)
		{
			SSL_free(ssl_);
		}
	}

	expected<std::shared_ptr<Http3Connection>, Error> Http3Connection::accept(
		net::DatagramSocket& socket, const net::TlsContext& tls, const net::Datagram& initial,
		const CidKey& client_dcid, const CidKey& client_scid, std::uint32_t version, std::size_t worker_index,
		RequestHandler handler)
	{
		auto connection =
			std::shared_ptr<Http3Connection>(new Http3Connection(socket, worker_index, std::move(handler)));

		connection->peer_ = initial.peer;
		connection->local_ = initial.local;

		// The server picks its own connection ID, and the worker index goes inside it.
		// If randomness is unavailable the connection is refused rather than issued a
		// guessable ID, because a guessable ID is an injection vector (RFC 9000 5.1).
		std::array<std::uint8_t, server_cid_length> scid_bytes{};
		if (!cid_fill(scid_bytes, worker_index))
		{
			return unexpected(quic_error("could not generate a connection ID"));
		}
		connection->scid_ = CidKey(scid_bytes.data(), scid_bytes.size());

		ngtcp2_cid scid{};
		ngtcp2_cid_init(&scid, scid_bytes.data(), scid_bytes.size());
		ngtcp2_cid dcid{};
		ngtcp2_cid_init(&dcid, client_scid.bytes.data(), client_scid.len);

		static constexpr auto callbacks = ngtcp2_callbacks{
			.recv_client_initial = ngtcp2_crypto_recv_client_initial_cb,
			.recv_crypto_data = ngtcp2_crypto_recv_crypto_data_cb,
			.handshake_completed = cb_handshake_completed,
			.encrypt = ngtcp2_crypto_encrypt_cb,
			.decrypt = ngtcp2_crypto_decrypt_cb,
			.hp_mask = ngtcp2_crypto_hp_mask_cb,
			.recv_stream_data = cb_recv_stream_data,
			.acked_stream_data_offset = cb_acked_stream_data_offset,
			.rand = cb_rand,
			.update_key = ngtcp2_crypto_update_key_cb,
			.stream_reset = cb_stream_reset,
			.delete_crypto_aead_ctx = ngtcp2_crypto_delete_crypto_aead_ctx_cb,
			.delete_crypto_cipher_ctx = ngtcp2_crypto_delete_crypto_cipher_ctx_cb,
			.version_negotiation = ngtcp2_crypto_version_negotiation_cb,
			.get_new_connection_id2 = cb_get_new_connection_id,
			.get_path_challenge_data2 = ngtcp2_crypto_get_path_challenge_data2_cb,
			.stream_close2 = cb_stream_close,
		};

		ngtcp2_settings settings;
		ngtcp2_settings_default(&settings);
		settings.initial_ts = now_ts();
		settings.max_tx_udp_payload_size = max_datagram_size;

		ngtcp2_transport_params params;
		ngtcp2_transport_params_default(&params);
		params.initial_max_stream_data_bidi_local = 256 * 1024;
		params.initial_max_stream_data_bidi_remote = 256 * 1024;
		params.initial_max_stream_data_uni = 256 * 1024;
		params.initial_max_data = 1024 * 1024;
		params.initial_max_streams_bidi = 100;
		// HTTP/3 needs three unidirectional streams from each side before it can send
		// anything at all: control, QPACK encoder, QPACK decoder. Advertising fewer
		// deadlocks the connection the moment the handshake finishes.
		params.initial_max_streams_uni = 3;
		params.max_idle_timeout = 30 * NGTCP2_SECONDS;
		// The client checks this against the connection ID it addressed its Initial to.
		// It is what proves the handshake was not redirected by someone in the middle.
		ngtcp2_cid_init(&params.original_dcid, client_dcid.bytes.data(), client_dcid.len);
		params.original_dcid_present = 1;

		ngtcp2_path path{};
		addr_from(path.local, connection->local_);
		addr_from(path.remote, connection->peer_);

		if (ngtcp2_conn_server_new(&connection->conn_, &dcid, &scid, &path, version, &callbacks, &settings, &params,
		                           nullptr, connection.get()) != 0)
		{
			return unexpected(quic_error("ngtcp2_conn_server_new failed"));
		}

		if (auto result = connection->setup_tls(tls); !result)
		{
			return unexpected(result.error());
		}

		// The Initial goes in through the ordinary path. From here on it is not a
		// special case, which is the point of doing it last.
		if (auto result = connection->read_packet(initial); !result)
		{
			return unexpected(result.error());
		}

		return connection;
	}

	expected<void, Error> Http3Connection::setup_tls(const net::TlsContext& tls)
	{
		ssl_ = SSL_new(tls.native_handle());
		if (ssl_ == nullptr)
		{
			return unexpected(quic_error("SSL_new failed"));
		}

		// ngtcp2 reaches the ngtcp2_conn from the SSL object through this back
		// reference, because OpenSSL callbacks only ever hand back the SSL.
		conn_ref_.get_conn = [](ngtcp2_crypto_conn_ref* ref)
		{ return static_cast<Http3Connection*>(ref->user_data)->conn_; };
		conn_ref_.user_data = this;
		SSL_set_app_data(ssl_, &conn_ref_);
		SSL_set_accept_state(ssl_);

		if (ngtcp2_crypto_ossl_ctx_new(&ossl_ctx_, ssl_) != 0)
		{
			return unexpected(quic_error("ngtcp2_crypto_ossl_ctx_new failed"));
		}
		// Per session, not per context: this installs the QUIC record layer on this one
		// SSL object, which is why it cannot be done once when the context is built.
		if (ngtcp2_crypto_ossl_configure_server_session(ssl_) != 0)
		{
			return unexpected(quic_error("ngtcp2_crypto_ossl_configure_server_session failed"));
		}

		ngtcp2_conn_set_tls_native_handle(conn_, ossl_ctx_);
		return {};
	}

	expected<void, Error> Http3Connection::setup_http3()
	{
		static constexpr nghttp3_callbacks h3_callbacks{
			.acked_stream_data = nullptr,
			.stream_close = cb_h3_stream_close,
			.recv_data = cb_h3_recv_data,
			.deferred_consume = cb_h3_deferred_consume,
			.begin_headers = cb_h3_begin_headers,
			.recv_header = cb_h3_recv_header,
			.end_headers = nullptr,
			.begin_trailers = nullptr,
			.recv_trailer = nullptr,
			.end_trailers = nullptr,
			.stop_sending = cb_h3_stop_sending,
			.end_stream = cb_h3_end_stream,
			.reset_stream = cb_h3_reset_stream,
		};

		nghttp3_settings h3_settings;
		nghttp3_settings_default(&h3_settings);
		h3_settings.qpack_max_dtable_capacity = 4096;
		h3_settings.qpack_blocked_streams = 100;

		if (nghttp3_conn_server_new(&h3_, &h3_callbacks, &h3_settings, nghttp3_mem_default(), this) != 0)
		{
			return unexpected(quic_error("nghttp3_conn_server_new failed"));
		}

		// HTTP/3 opens three unidirectional streams before anything else can happen.
		// ngtcp2 hands out the stream IDs; nghttp3 says what each one is for.
		std::int64_t control = -1;
		if (ngtcp2_conn_open_uni_stream(conn_, &control, nullptr) != 0 ||
		    nghttp3_conn_bind_control_stream(h3_, control) != 0)
		{
			return unexpected(quic_error("could not open the HTTP/3 control stream"));
		}

		std::int64_t qpack_encoder = -1;
		std::int64_t qpack_decoder = -1;
		if (ngtcp2_conn_open_uni_stream(conn_, &qpack_encoder, nullptr) != 0 ||
		    ngtcp2_conn_open_uni_stream(conn_, &qpack_decoder, nullptr) != 0 ||
		    nghttp3_conn_bind_qpack_streams(h3_, qpack_encoder, qpack_decoder) != 0)
		{
			return unexpected(quic_error("could not open the QPACK streams"));
		}

		return {};
	}

	// ========================================================================
	// Packet in, packets out
	// ========================================================================

	expected<void, Error> Http3Connection::read_packet(const net::Datagram& datagram)
	{
		if (closed_)
		{
			return unexpected(quic_error("connection is closed"));
		}

		ngtcp2_path path{};
		addr_from(path.local, datagram.local);
		addr_from(path.remote, datagram.peer);

		const ngtcp2_pkt_info packet_info{.ecn = datagram.ecn};

		if (ngtcp2_conn_read_pkt(conn_, &path, &packet_info, datagram.data.data(), datagram.data.size(), now_ts()) != 0)
		{
			closed_ = true;
			return unexpected(quic_error("ngtcp2_conn_read_pkt failed"));
		}
		return {};
	}

	Task<expected<void, Error>> Http3Connection::flush()
	{
		if (closed_)
		{
			co_return unexpected(quic_error("connection is closed"));
		}

		// A flush already in progress will pick this up before it returns. Returning
		// here rather than waiting is what keeps the two callers from interleaving
		// inside ngtcp2, which is not reentrant.
		if (flushing_)
		{
			flush_pending_ = true;
			co_return expected<void, Error>{};
		}

		flushing_ = true;
		// Cleared on every exit, including the error paths below, which is why it is a
		// guard object rather than an assignment before each co_return.
		struct FlushGuard
		{
			bool& flag;
			~FlushGuard() { flag = false; }
		} guard{flushing_};

		std::array<std::uint8_t, max_datagram_size> buffer{};

		do
		{
			// Anything that arrived while the previous pass was suspended is picked up
			// by the next turn of the outer loop.
			flush_pending_ = false;

			for (;;)
			{
				std::int64_t stream_id = -1;
				int fin = 0;
				std::array<nghttp3_vec, max_write_vecs> vecs{};
				nghttp3_ssize vec_count = 0;

				if (h3_ != nullptr)
				{
					vec_count = nghttp3_conn_writev_stream(h3_, &stream_id, &fin, vecs.data(), vecs.size());
					if (vec_count < 0)
					{
						closed_ = true;
						co_return unexpected(quic_error("nghttp3_conn_writev_stream failed"));
					}
				}

				std::uint32_t flags = NGTCP2_WRITE_STREAM_FLAG_MORE;
				if (fin != 0)
				{
					flags |= NGTCP2_WRITE_STREAM_FLAG_FIN;
				}

				ngtcp2_path_storage path_storage;
				ngtcp2_path_storage_zero(&path_storage);
				ngtcp2_pkt_info packet_info{};
				ngtcp2_ssize written_data = 0;

				const ngtcp2_ssize written = ngtcp2_conn_writev_stream(
					conn_, &path_storage.path, &packet_info, buffer.data(), buffer.size(), &written_data, flags, stream_id,
					reinterpret_cast<const ngtcp2_vec*>(vecs.data()), vec_count, now_ts());

				if (written < 0)
				{
					// Two of these are ordinary flow-control outcomes rather than errors,
					// and each resumes differently, so they are separated rather than
					// collapsed into one retry.
					if (written == NGTCP2_ERR_WRITE_MORE)
					{
						// The packet has room left. Tell nghttp3 what was taken and go round
						// again to fill it.
						if (written_data > 0 &&
						    nghttp3_conn_add_write_offset(h3_, stream_id, static_cast<std::size_t>(written_data)) != 0)
						{
							closed_ = true;
							co_return unexpected(quic_error("nghttp3_conn_add_write_offset failed"));
						}
						continue;
					}
					if (written == NGTCP2_ERR_STREAM_DATA_BLOCKED || written == NGTCP2_ERR_STREAM_SHUT_WR)
					{
						// This stream cannot send now, but the others may. Park it so
						// nghttp3 stops offering it, and keep going.
						if (stream_id >= 0)
						{
							nghttp3_conn_block_stream(h3_, stream_id);
						}
						continue;
					}
					closed_ = true;
					co_return unexpected(quic_error("ngtcp2_conn_writev_stream failed"));
				}

				if (written_data > 0 &&
				    nghttp3_conn_add_write_offset(h3_, stream_id, static_cast<std::size_t>(written_data)) != 0)
				{
					closed_ = true;
					co_return unexpected(quic_error("nghttp3_conn_add_write_offset failed"));
				}

				// Zero means ngtcp2 has nothing further to send right now.
				if (written == 0)
				{
					break;
				}

				// The path comes back from ngtcp2 rather than being remembered, so a client
				// that has migrated is answered at its new address with no special case.
				peer_ = endpoint_from(path_storage.path.remote);
				local_ = endpoint_from(path_storage.path.local);

				auto sent = co_await socket_.async_send({buffer.data(), static_cast<std::size_t>(written)}, peer_, local_);
				if (!sent)
				{
					co_return unexpected(sent.error());
				}
			}
		} while (flush_pending_);

		co_return expected<void, Error>{};
	}

	ngtcp2_tstamp Http3Connection::expiry() const noexcept
	{
		return conn_ == nullptr ? UINT64_MAX : ngtcp2_conn_get_expiry(conn_);
	}

	expected<void, Error> Http3Connection::handle_expiry()
	{
		if (closed_ || conn_ == nullptr)
		{
			return unexpected(quic_error("connection is closed"));
		}
		if (ngtcp2_conn_handle_expiry(conn_, now_ts()) != 0)
		{
			closed_ = true;
			return unexpected(Error::timeout());
		}
		return {};
	}

	// ========================================================================
	// Transport callbacks
	// ========================================================================

	int Http3Connection::on_handshake_completed()
	{
		handshake_done_ = true;
		// HTTP/3 is brought up only now, because its three control streams cannot be
		// opened until the transport parameters have been exchanged.
		if (!setup_http3())
		{
			return NGTCP2_ERR_CALLBACK_FAILURE;
		}
		return 0;
	}

	int Http3Connection::on_new_connection_id(ngtcp2_cid* cid, ngtcp2_stateless_reset_token* token, std::size_t cidlen)
	{
		if (cidlen > max_cid_length)
		{
			return NGTCP2_ERR_CALLBACK_FAILURE;
		}
		// Every additional connection ID carries the same worker index, so a client
		// that rotates its connection ID still lands on the worker holding its state.
		if (!cid_fill({cid->data, cidlen}, worker_index_))
		{
			return NGTCP2_ERR_CALLBACK_FAILURE;
		}
		cid->datalen = cidlen;

		static_assert(stateless_reset_token_length == NGTCP2_STATELESS_RESET_TOKENLEN,
		              "the stateless reset token width is fixed by RFC 9000 and must match ngtcp2's");
		const auto derived = derive_reset_token(server_reset_secret(), CidKey(cid->data, cidlen));
		std::memcpy(token->data, derived.data(), derived.size());
		return 0;
	}

	int Http3Connection::on_recv_stream_data(std::uint32_t flags, std::int64_t stream_id, const std::uint8_t* data,
	                                         std::size_t len)
	{
		if (h3_ == nullptr)
		{
			return 0;
		}
		const int fin = (flags & NGTCP2_STREAM_DATA_FLAG_FIN) != 0 ? 1 : 0;
		const nghttp3_ssize consumed = nghttp3_conn_read_stream(h3_, stream_id, data, len, fin);
		if (consumed < 0)
		{
			return NGTCP2_ERR_CALLBACK_FAILURE;
		}
		// Both layers track flow control, so both have to be told what was consumed, or
		// the connection stalls once the window closes.
		ngtcp2_conn_extend_max_stream_offset(conn_, stream_id, static_cast<std::uint64_t>(consumed));
		ngtcp2_conn_extend_max_offset(conn_, static_cast<std::uint64_t>(consumed));
		return 0;
	}

	int Http3Connection::on_acked_stream_data(std::int64_t stream_id, std::uint64_t datalen)
	{
		if (h3_ != nullptr && nghttp3_conn_add_ack_offset(h3_, stream_id, datalen) != 0)
		{
			return NGTCP2_ERR_CALLBACK_FAILURE;
		}
		return 0;
	}

	int Http3Connection::on_stream_close(std::int64_t stream_id, std::uint64_t app_error_code)
	{
		if (h3_ != nullptr && nghttp3_conn_close_stream(h3_, stream_id, app_error_code) != 0)
		{
			return NGTCP2_ERR_CALLBACK_FAILURE;
		}
		return 0;
	}

	// ========================================================================
	// HTTP/3 callbacks
	// ========================================================================

	Http3Connection::Stream* Http3Connection::find_stream(std::int64_t stream_id) noexcept
	{
		const auto it = streams_.find(stream_id);
		return it == streams_.end() ? nullptr : &it->second;
	}

	int Http3Connection::on_h3_recv_header(std::int64_t stream_id, std::span<const std::uint8_t> name,
	                                       std::span<const std::uint8_t> value)
	{
		auto& stream = streams_[stream_id];
		// A rejected field is not fatal here: the builder records why, and dispatch
		// turns that into a 400 so the client learns what was wrong.
		stream.builder.add({reinterpret_cast<const char*>(name.data()), name.size()},
		                   {reinterpret_cast<const char*>(value.data()), value.size()});
		return 0;
	}

	int Http3Connection::on_h3_recv_data(std::int64_t stream_id, std::span<const std::uint8_t> data)
	{
		auto& stream = streams_[stream_id];
		stream.body.append(reinterpret_cast<const char*>(data.data()), data.size());
		return 0;
	}

	int Http3Connection::on_h3_end_stream(std::int64_t stream_id)
	{
		// The handler is a coroutine and may take a while. Detaching keeps the
		// transport reading meanwhile, which is the whole point of a multiplexed
		// protocol: one slow handler must not stall the other streams.
		dispatch(stream_id).start_detached();
		return 0;
	}

	int Http3Connection::on_h3_stream_close(std::int64_t stream_id)
	{
		streams_.erase(stream_id);
		return 0;
	}

	Task<void> Http3Connection::dispatch(std::int64_t stream_id)
	{
		// Keeps the connection alive for as long as the handler runs. Without it a
		// client that disappears mid-request would free the object out from under this
		// coroutine, which is detached and therefore has no other owner.
		auto keepalive = shared_from_this();

		auto* stream = find_stream(stream_id);
		if (stream == nullptr || h3_ == nullptr)
		{
			co_return;
		}

		auto request = stream->builder.build();
		if (!request)
		{
			// Malformed per RFC 9114 section 4.2. Answered rather than dropped, so the
			// client learns its request was rejected instead of waiting for a timeout.
			stream->response = Response::bad_request(std::string(describe(stream->builder.error())));
		}
		else
		{
			request->set_body(std::move(stream->body));
			stream->response = co_await handler_(*request);
		}

		stream->fields = response_fields(stream->response);
		stream->nva.clear();
		stream->nva.reserve(stream->fields.size());
		for (auto& field : stream->fields)
		{
			stream->nva.push_back(nghttp3_nv{
				.name = reinterpret_cast<std::uint8_t*>(field.name.data()),
				.value = reinterpret_cast<std::uint8_t*>(field.value.data()),
				.namelen = field.name.size(),
				.valuelen = field.value.size(),
				.flags = NGHTTP3_NV_FLAG_NONE,
			});
		}

		// The body is read back lazily, so nghttp3 is pointed at the Stream's own copy
		// rather than at anything owned by this coroutine frame. The request body is
		// finished with by now, so that buffer is reused.
		stream->body = std::string(stream->response.body());

		const nghttp3_data_reader reader{.read_data = cb_h3_read_data};
		if (nghttp3_conn_submit_response(h3_, stream_id, stream->nva.data(), stream->nva.size(), &reader) != 0)
		{
			closed_ = true;
			co_return;
		}
		nghttp3_conn_set_stream_user_data(h3_, stream_id, &stream->body);

		// Nothing else will run the write loop: the packet that carried this request
		// was answered long ago, and this coroutine is what resumed last.
		(void)co_await flush();
	}

}  // namespace coroute::http3

#endif  // COROUTE_HAS_HTTP3
