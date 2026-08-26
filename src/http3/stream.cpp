// Http3Stream - per-request state and response submission for HTTP/3.

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
#include "coroute/http3/zero_rtt_replay_cache.hpp"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <string>

namespace coroute::http3
{

namespace
{

// nghttp3 data reader: pull the response body out of the owning Http3Stream.
nghttp3_ssize read_response_body_cb(nghttp3_conn* /*conn*/, int64_t /*stream_id*/,
                                    nghttp3_vec* vec, size_t veccnt, uint32_t* pflags,
                                    void* /*conn_user_data*/, void* stream_user_data)
{
	if (veccnt == 0) return 0;
	auto* self = static_cast<Http3Stream*>(stream_user_data);
	if (!self)
	{
		*pflags |= NGHTTP3_DATA_FLAG_EOF;
		return 0;
	}

	bool eof = false;
	size_t remaining = 0;
	uint8_t* base = self->pull_body_range(&remaining, &eof);
	if (eof) *pflags |= NGHTTP3_DATA_FLAG_EOF;
	if (!base || remaining == 0)
	{
		return 0;
	}
	vec[0].base = base;
	vec[0].len = remaining;
	return 1;
}

}  // anonymous namespace

// ===========================================================================
// Http3Stream
// ===========================================================================

Http3Stream::Http3Stream(Http3Connection* conn, int64_t stream_id,
                         bool received_in_0rtt)
    : conn_(conn), stream_id_(stream_id), received_in_0rtt_(received_in_0rtt)
{
}

void Http3Stream::on_header(std::string name, std::string value)
{
	// HTTP/3 pseudo-headers start with ':'. Map them into the Request model
	// instead of stashing them as regular headers.
	if (!name.empty() && name.front() == ':')
	{
		if (name == ":method")
		{
			req_.set_method(std::string_view{value.data(), value.size()});
		}
		else if (name == ":path")
		{
			auto qpos = value.find('?');
			if (qpos == std::string::npos)
			{
				req_.set_path(std::move(value));
			}
			else
			{
				req_.set_path(value.substr(0, qpos));
				req_.set_query_string(value.substr(qpos + 1));
			}
		}
		else if (name == ":authority")
		{
			req_.add_header("Host", std::move(value));
		}
		// :scheme is informational — always "https" under QUIC.
		return;
	}
	req_.add_header(std::move(name), std::move(value));
}

void Http3Stream::on_body(const uint8_t* data, size_t len)
{
	// Delegate to the chunked arena. Avoids O(n²) copies from repeated
	// std::string::append-with-realloc on large uploads.
	append_body_chunk(data, len);
}

void Http3Stream::append_body_chunk(const uint8_t* data, size_t len)
{
	// Fill the current partial page, then allocate new pages as needed.
	// No existing data is ever moved — each page is independent.
	while (len > 0)
	{
		const bool need_new_page =
		    body_chunks_.empty() || body_last_chunk_used_ == kBodyChunkSize;

		if (need_new_page)
		{
			body_chunks_.emplace_back();
			body_last_chunk_used_ = 0;
		}

		const std::size_t space    = kBodyChunkSize - body_last_chunk_used_;
		const std::size_t to_copy  = std::min(len, space);
		std::memcpy(body_chunks_.back().data() + body_last_chunk_used_, data, to_copy);
		body_last_chunk_used_ += to_copy;
		data += to_copy;
		len  -= to_copy;
	}
}

void Http3Stream::coalesce_body_to_request()
{
	if (body_chunks_.empty()) return;

	// Compute total byte count to reserve once — avoids repeated string growth.
	std::size_t total = 0;
	for (std::size_t i = 0; i < body_chunks_.size(); ++i)
	{
		total += (i + 1 < body_chunks_.size()) ? kBodyChunkSize : body_last_chunk_used_;
	}

	std::string body;
	body.reserve(total);

	for (std::size_t i = 0; i < body_chunks_.size(); ++i)
	{
		const std::size_t n =
		    (i + 1 < body_chunks_.size()) ? kBodyChunkSize : body_last_chunk_used_;
		body.append(reinterpret_cast<const char*>(body_chunks_[i].data()), n);
	}

	req_.set_body(std::move(body));

	// Release arena memory — coalesced into req_ which owns the data now.
	body_chunks_.clear();
	body_last_chunk_used_ = 0;
}

void Http3Stream::on_end_headers()
{
	// Request body (if any) arrives after headers via on_body.
}

void Http3Stream::on_end_stream()
{
	// Coalesce the chunked request-body arena into req_ exactly once, before
	// we inspect req_.body() for the replay-token check or hand the Request
	// to the application handler.
	coalesce_body_to_request();

	auto* quic = conn_->quic_conn();
	auto* server = quic ? quic->server() : nullptr;
	if (!server) return;

	if (!server->has_handler())
	{
		Response r = Response::not_found();
		send_response(r);
		return;
	}

	// RFC 8446 §8.1 / RFC 8470: reject 0-RTT requests whose token has been
	// seen before (replay attack). Respond 425 Too Early so the client can
	// retry on the established 1-RTT connection.
	if (received_in_0rtt_)
	{
		const std::string method_str{method_to_string(req_.method())};
		// Use the server-chosen SCID (unique per QUIC connection) as the
		// peer identity so cross-connection replays produce a distinct token.
		const std::string& peer_scid = conn_->quic_conn()->cid_key();
		const std::string token = ZeroRttReplayCache::make_token(
		    peer_scid, method_str, req_.path(), req_.body());
		if (ZeroRttReplayCache::instance().check_and_mark(token))
		{
			Response r;
			r.set_status(425);  // 425 Too Early
			r.set_body("425 Too Early — 0-RTT replay detected");
			send_response(r);
			return;
		}
	}

	auto handler = server->handler();
	int64_t sid = stream_id_;
	auto* h3conn = conn_;
	Request req = std::move(req_);

	// Detached coroutine: awaits the app handler, then submits the response.
	// When it resolves we look up the stream again — it may have been closed
	// concurrently (e.g. client RST_STREAM), in which case we just drop the
	// result.
	[h3conn, sid, handler = std::move(handler), req = std::move(req)]() mutable -> Task<void>
	{
		Response resp;
		try
		{
			resp = co_await handler(std::move(req));
		}
		catch (...)
		{
			resp = Response::internal_error();
		}
		if (auto* s = h3conn->find_stream(sid))
		{
			s->send_response(resp);
		}
		co_return;
	}()
	    .start_detached();
}

void Http3Stream::send_response(const coroute::Response& res)
{
	if (response_submitted_) return;
	response_submitted_ = true;

	auto* h3conn = conn_->http3_conn();
	if (!h3conn) return;

	// 1) Own header storage for the lifetime of this stream.
	hdr_storage_.clear();
	hdr_storage_.reserve((res.headers().size() + 1) * 2);

	auto push_hdr = [&](std::string name, std::string value)
	{
		hdr_storage_.push_back(std::move(name));
		hdr_storage_.push_back(std::move(value));
	};

	push_hdr(":status", std::to_string(res.status()));
	for (const auto& [k, v] : res.headers())
	{
		// Lowercase header names (HTTP/3 / QPACK requires this).
		std::string lname = k;
		std::transform(lname.begin(), lname.end(), lname.begin(),
		               [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		push_hdr(std::move(lname), v);
	}

	// Make sure content-length matches the body we're about to send, unless
	// the app already set it.
	bool has_cl = false;
	for (size_t i = 0; i + 1 < hdr_storage_.size(); i += 2)
	{
		if (hdr_storage_[i] == "content-length")
		{
			has_cl = true;
			break;
		}
	}
	if (!has_cl)
	{
		push_hdr("content-length", std::to_string(res.body().size()));
	}

	// 2) Snapshot body into body_buffer_ — owned and stable for the read_data
	//    callback's lifetime.
	body_buffer_.assign(res.body().begin(), res.body().end());
	body_offset_ = 0;

	// 3) Build the nghttp3_nv array. It's constructed into a local vector
	//    because nghttp3_conn_submit_response copies the fields immediately
	//    (only name/value pointers need to stay alive — which they do, via
	//    hdr_storage_).
	std::vector<nghttp3_nv> nva;
	nva.reserve(hdr_storage_.size() / 2);
	for (size_t i = 0; i + 1 < hdr_storage_.size(); i += 2)
	{
		nghttp3_nv nv{};
		nv.name = reinterpret_cast<const uint8_t*>(hdr_storage_[i].data());
		nv.namelen = hdr_storage_[i].size();
		nv.value = reinterpret_cast<const uint8_t*>(hdr_storage_[i + 1].data());
		nv.valuelen = hdr_storage_[i + 1].size();
		nv.flags = NGHTTP3_NV_FLAG_NONE;
		nva.push_back(nv);
	}

	// 4) Install the data reader pointing back at us via stream_user_data.
	//    stream_user_data was set up in Http3Connection::on_stream_data.
	nghttp3_data_reader reader{};
	reader.read_data = read_response_body_cb;

	int rv = nghttp3_conn_submit_response(h3conn, stream_id_, nva.data(), nva.size(),
	                                      body_buffer_.empty() ? nullptr : &reader);
	if (rv != 0)
	{
		std::cerr << "HTTP/3: nghttp3_conn_submit_response failed: "
		          << nghttp3_strerror(rv) << std::endl;
		return;
	}

	// 5) Wake the write loop to flush this stream.
	if (auto* quic = conn_->quic_conn())
	{
		quic->schedule_write();
	}
}

uint8_t* Http3Stream::pull_body_range(size_t* remaining, bool* eof)
{
	if (body_offset_ >= body_buffer_.size())
	{
		*remaining = 0;
		*eof = true;
		return nullptr;
	}
	size_t n = body_buffer_.size() - body_offset_;
	uint8_t* p = reinterpret_cast<uint8_t*>(body_buffer_.data()) + body_offset_;
	// Hand the remainder off in one shot. nghttp3 will call us again if more
	// is needed — but since we've handed all bytes, next call returns eof.
	body_offset_ = body_buffer_.size();
	*remaining = n;
	*eof = true;
	return p;
}

}  // namespace coroute::http3
