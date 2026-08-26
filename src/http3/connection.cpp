// Http3Connection - bridge between ngtcp2 QUIC streams and nghttp3.

#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <winsock2.h>
#endif
#ifdef DELETE
#  undef DELETE
#endif

#include <ngtcp2/ngtcp2.h>
#include <nghttp3/nghttp3.h>

#include "coroute/http3/quic_server.hpp"

#include <chrono>
#include <iostream>

namespace coroute::http3
{

namespace
{

Http3Stream* h3_stream_from(void* stream_user_data)
{
	return static_cast<Http3Stream*>(stream_user_data);
}

Http3Connection* h3_from(void* conn_user_data)
{
	return static_cast<Http3Connection*>(conn_user_data);
}

// Server-side request streams are not created in nghttp3 until the first
// frame is parsed, so our earlier attempt to install stream_user_data from
// Http3Connection::on_stream_data misses: the callbacks fire with a null
// pointer. Resolve the stream lazily on first use and install user_data so
// later callbacks in the same batch (and in subsequent datagrams) receive
// the correct pointer.
Http3Stream* ensure_stream_user_data(void* conn_user_data,
                                     void* stream_user_data,
                                     int64_t stream_id,
                                     nghttp3_conn* http3_conn)
{
	if (auto* existing = static_cast<Http3Stream*>(stream_user_data)) return existing;
	auto* h3 = h3_from(conn_user_data);
	if (!h3) return nullptr;
	Http3Stream* s = h3->get_or_create_stream(stream_id);
	if (s) nghttp3_conn_set_stream_user_data(http3_conn, stream_id, s);
	return s;
}

uint64_t monotonic_tstamp_ns()
{
	using namespace std::chrono;
	return static_cast<uint64_t>(
	    duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count());
}

// ---- nghttp3 callback trampolines ----------------------------------------

int h3_cb_recv_data(nghttp3_conn* h3c, int64_t stream_id, const uint8_t* data, size_t datalen,
                    void* conn_user_data, void* stream_user_data)
{
	auto* s = ensure_stream_user_data(conn_user_data, stream_user_data, stream_id, h3c);
	if (s) s->on_body(data, datalen);
	return 0;
}

int h3_cb_recv_header(nghttp3_conn* h3c, int64_t stream_id, int32_t /*token*/,
                      nghttp3_rcbuf* name, nghttp3_rcbuf* value, uint8_t /*flags*/,
                      void* conn_user_data, void* stream_user_data)
{
	auto* s = ensure_stream_user_data(conn_user_data, stream_user_data, stream_id, h3c);
	if (!s) return 0;
	auto nbuf = nghttp3_rcbuf_get_buf(name);
	auto vbuf = nghttp3_rcbuf_get_buf(value);
	s->on_header(
	    std::string(reinterpret_cast<const char*>(nbuf.base), nbuf.len),
	    std::string(reinterpret_cast<const char*>(vbuf.base), vbuf.len));
	return 0;
}

int h3_cb_end_headers(nghttp3_conn* h3c, int64_t stream_id, int /*fin*/,
                      void* conn_user_data, void* stream_user_data)
{
	auto* s = ensure_stream_user_data(conn_user_data, stream_user_data, stream_id, h3c);
	if (s) s->on_end_headers();
	return 0;
}

int h3_cb_end_stream(nghttp3_conn* h3c, int64_t stream_id,
                     void* conn_user_data, void* stream_user_data)
{
	auto* s = ensure_stream_user_data(conn_user_data, stream_user_data, stream_id, h3c);
	if (s) s->on_end_stream();
	return 0;
}

int h3_cb_deferred_consume(nghttp3_conn*, int64_t stream_id, size_t consumed,
                           void* conn_user_data, void* /*stream_user_data*/)
{
	auto* h3 = h3_from(conn_user_data);
	if (!h3) return 0;
	if (auto* quic = h3->quic_conn())
	{
		if (auto* c = quic->quic_conn())
		{
			ngtcp2_conn_extend_max_stream_offset(c, stream_id, consumed);
			ngtcp2_conn_extend_max_offset(c, consumed);
		}
	}
	return 0;
}

int h3_cb_stop_sending(nghttp3_conn*, int64_t stream_id, uint64_t app_error_code,
                       void*, void* stream_user_data)
{
	auto* s = h3_stream_from(stream_user_data);
	if (!s) return 0;
	if (auto* quic = s->connection()->quic_conn())
	{
		if (auto* c = quic->quic_conn())
		{
			ngtcp2_conn_shutdown_stream_read(c, 0, stream_id, app_error_code);
		}
	}
	return 0;
}

int h3_cb_reset_stream(nghttp3_conn*, int64_t stream_id, uint64_t app_error_code,
                       void*, void* stream_user_data)
{
	auto* s = h3_stream_from(stream_user_data);
	if (!s) return 0;
	if (auto* quic = s->connection()->quic_conn())
	{
		if (auto* c = quic->quic_conn())
		{
			ngtcp2_conn_shutdown_stream_write(c, 0, stream_id, app_error_code);
		}
	}
	return 0;
}

int h3_cb_stream_close(nghttp3_conn*, int64_t stream_id, uint64_t /*app_error_code*/,
                       void* conn_user_data, void* /*stream_user_data*/)
{
	auto* h3 = h3_from(conn_user_data);
	if (h3) h3->remove_stream(stream_id);
	return 0;
}

int h3_cb_acked_stream_data(nghttp3_conn*, int64_t /*stream_id*/, uint64_t /*datalen*/,
                            void*, void*)
{
	return 0;
}

int h3_cb_recv_settings(nghttp3_conn*, const nghttp3_settings*, void*)
{
	return 0;
}

}  // anonymous namespace

// ===========================================================================
// Http3Connection
// ===========================================================================

Http3Connection::Http3Connection(QuicConnection* quic_conn)
    : quic_conn_(quic_conn)
{
	nghttp3_callbacks callbacks{};
	callbacks.acked_stream_data = h3_cb_acked_stream_data;
	callbacks.stream_close = h3_cb_stream_close;
	callbacks.recv_data = h3_cb_recv_data;
	callbacks.deferred_consume = h3_cb_deferred_consume;
	callbacks.recv_header = h3_cb_recv_header;
	callbacks.end_headers = h3_cb_end_headers;
	callbacks.recv_trailer = h3_cb_recv_header;    // reuse
	callbacks.end_trailers = h3_cb_end_headers;    // reuse
	callbacks.stop_sending = h3_cb_stop_sending;
	callbacks.end_stream = h3_cb_end_stream;
	callbacks.reset_stream = h3_cb_reset_stream;
	callbacks.recv_settings = h3_cb_recv_settings;

	nghttp3_settings settings{};
	nghttp3_settings_default(&settings);

	int rv = nghttp3_conn_server_new(&http3_conn_, &callbacks, &settings,
	                                  nghttp3_mem_default(), this);
	if (rv != 0)
	{
		std::cerr << "HTTP/3: nghttp3_conn_server_new failed: "
		          << nghttp3_strerror(rv) << std::endl;
		http3_conn_ = nullptr;
	}
}

Http3Connection::~Http3Connection()
{
	if (http3_conn_)
	{
		nghttp3_conn_del(http3_conn_);
		http3_conn_ = nullptr;
	}
}

int Http3Connection::setup_server_streams()
{
	if (!http3_conn_ || !quic_conn_ || !quic_conn_->quic_conn()) return -1;

	ngtcp2_conn* qc = quic_conn_->quic_conn();
	int64_t ctrl_id = -1;
	int64_t qenc_id = -1;
	int64_t qdec_id = -1;

	int rv = ngtcp2_conn_open_uni_stream(qc, &ctrl_id, nullptr);
	if (rv != 0) return rv;
	rv = ngtcp2_conn_open_uni_stream(qc, &qenc_id, nullptr);
	if (rv != 0) return rv;
	rv = ngtcp2_conn_open_uni_stream(qc, &qdec_id, nullptr);
	if (rv != 0) return rv;

	rv = nghttp3_conn_bind_control_stream(http3_conn_, ctrl_id);
	if (rv != 0) return rv;
	rv = nghttp3_conn_bind_qpack_streams(http3_conn_, qenc_id, qdec_id);
	if (rv != 0) return rv;

	return 0;
}

long long Http3Connection::on_stream_data(int64_t stream_id, const uint8_t* data,
                                          size_t datalen, bool fin,
                                          bool received_in_0rtt)
{
	if (!http3_conn_) return 0;

	// Pre-create our Http3Stream object; the nghttp3 callbacks below will
	// bind it as stream_user_data lazily (via ensure_stream_user_data), since
	// calling nghttp3_conn_set_stream_user_data before nghttp3 has internally
	// created the stream is a no-op.
	if (Http3Stream* s = get_or_create_stream(stream_id, received_in_0rtt))
	{
		// Propagate 0-RTT flag on every chunk — the first data frame of a
		// 0-RTT stream arrives with the flag set; subsequent retransmissions
		// (in 1-RTT) may not. We only mark, never unmark.
		if (received_in_0rtt) s->mark_received_in_0rtt();
	}

	nghttp3_ssize consumed = nghttp3_conn_read_stream2(
	    http3_conn_, stream_id, data, datalen, fin ? 1 : 0, monotonic_tstamp_ns());
	return static_cast<long long>(consumed);
}

int Http3Connection::on_stream_close(int64_t stream_id, uint64_t app_error_code)
{
	if (!http3_conn_) return 0;
	return nghttp3_conn_close_stream(http3_conn_, stream_id, app_error_code);
}

int Http3Connection::on_stream_reset(int64_t stream_id)
{
	if (!http3_conn_) return 0;
	return nghttp3_conn_shutdown_stream_read(http3_conn_, stream_id);
}

int Http3Connection::on_stream_stop_sending(int64_t /*stream_id*/)
{
	return 0;
}

int Http3Connection::on_acked_stream_data(int64_t /*stream_id*/, uint64_t /*datalen*/)
{
	return 0;
}

Http3Stream* Http3Connection::get_or_create_stream(int64_t stream_id,
                                                    bool received_in_0rtt)
{
	auto it = streams_.find(stream_id);
	if (it != streams_.end()) return it->second.get();

	// Only request streams (client-initiated bidi) need an Http3Stream.
	// Client-initiated bidi streams have (stream_id & 0x03) == 0.
	if ((stream_id & 0x03) != 0)
	{
		return nullptr;
	}
	auto s = std::make_unique<Http3Stream>(this, stream_id, received_in_0rtt);
	auto* raw = s.get();
	streams_.emplace(stream_id, std::move(s));
	return raw;
}

Http3Stream* Http3Connection::find_stream(int64_t stream_id)
{
	auto it = streams_.find(stream_id);
	return (it == streams_.end()) ? nullptr : it->second.get();
}

void Http3Connection::remove_stream(int64_t stream_id)
{
	streams_.erase(stream_id);
}

}  // namespace coroute::http3
