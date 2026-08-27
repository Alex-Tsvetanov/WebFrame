#include "coroute/net/protocol_detect.hpp"

#include <array>
#include <cstring>
#include <utility>

namespace coroute::net
{

	// ============================================================================
	// PrefaceConnection
	// ============================================================================

	PrefaceConnection::PrefaceConnection(std::unique_ptr<Connection> inner, std::vector<std::uint8_t> pushback)
		: inner_(std::move(inner)), pushback_(std::move(pushback))
	{
	}

	size_t PrefaceConnection::drain(void* buffer, size_t len) noexcept
	{
		const size_t available = pushback_.size() - pos_;
		const size_t n = std::min(available, len);
		if (n > 0)
		{
			std::memcpy(buffer, pushback_.data() + pos_, n);
			pos_ += n;
		}
		// clear() only, no shrink_to_fit: this function is noexcept and shrink_to_fit
		// may reallocate, which would terminate rather than throw. The buffer is at
		// most the 24 byte preface, so reclaiming its capacity is not worth the risk.
		if (pos_ >= pushback_.size() && !pushback_.empty())
		{
			pushback_.clear();
			pos_ = 0;
		}
		return n;
	}

	Task<ReadResult> PrefaceConnection::async_read(void* buffer, size_t len)
	{
		if (has_pushback())
		{
			// Deliberately does not top up from the socket afterwards. A short read is
			// valid for async_read, and mixing replayed bytes with a fresh read would
			// turn one call into two syscalls' worth of latency for no benefit.
			co_return drain(buffer, len);
		}
		co_return co_await inner_->async_read(buffer, len);
	}

	Task<ReadResult> PrefaceConnection::async_read_until(void* buffer, size_t len, char delimiter)
	{
		if (!has_pushback())
		{
			co_return co_await inner_->async_read_until(buffer, len, delimiter);
		}

		auto* out = static_cast<std::uint8_t*>(buffer);
		const size_t available = pushback_.size() - pos_;
		const size_t window = std::min(available, len);

		// If the delimiter is already among the replayed bytes, the call is satisfied
		// without touching the socket.
		const auto* found = static_cast<const std::uint8_t*>(std::memchr(pushback_.data() + pos_, delimiter, window));
		if (found != nullptr)
		{
			const size_t through = static_cast<size_t>(found - (pushback_.data() + pos_)) + 1;
			co_return drain(out, through);
		}

		// Otherwise replay everything we have and let the socket supply the rest into
		// the same buffer, so the caller still sees one contiguous result.
		const size_t replayed = drain(out, window);
		if (replayed == len)
		{
			co_return replayed;
		}

		auto rest = co_await inner_->async_read_until(out + replayed, len - replayed, delimiter);
		if (!rest)
		{
			co_return unexpected(rest.error());
		}
		co_return replayed + *rest;
	}

	Task<WriteResult> PrefaceConnection::async_write(const void* buffer, size_t len)
	{
		co_return co_await inner_->async_write(buffer, len);
	}

	Task<WriteResult> PrefaceConnection::async_write_all(const void* buffer, size_t len)
	{
		co_return co_await inner_->async_write_all(buffer, len);
	}

	Task<TransmitResult> PrefaceConnection::async_transmit_file(FileHandle file, size_t offset, size_t length)
	{
		co_return co_await inner_->async_transmit_file(file, offset, length);
	}

	void PrefaceConnection::close() { inner_->close(); }

	bool PrefaceConnection::is_open() const noexcept { return inner_->is_open(); }

	void PrefaceConnection::set_timeout(std::chrono::milliseconds timeout) { inner_->set_timeout(timeout); }

	std::string PrefaceConnection::remote_address() const { return inner_->remote_address(); }

	uint16_t PrefaceConnection::remote_port() const noexcept { return inner_->remote_port(); }

	void PrefaceConnection::set_cancellation_token(CancellationToken token)
	{
		inner_->set_cancellation_token(std::move(token));
	}

	// ============================================================================
	// read_prefix
	// ============================================================================

	Task<expected<Prefix, Error>> read_prefix(std::unique_ptr<Connection> conn, size_t min_bytes)
	{
		if (!conn)
		{
			co_return unexpected(Error::io(IoError::InvalidArgument, "read_prefix: null connection"));
		}

		std::vector<std::uint8_t> bytes;
		bytes.reserve(std::max<size_t>(min_bytes, http2_client_preface.size()));

		// Sized to the largest prefix any caller asks for, so the common case is one
		// read. A peer sending one byte at a time still terminates, because every
		// caller's min_bytes is bounded by the preface length.
		std::array<std::uint8_t, 64> chunk{};

		while (bytes.size() < min_bytes)
		{
			auto read = co_await conn->async_read(chunk.data(), chunk.size());
			if (!read)
			{
				co_return unexpected(read.error());
			}
			if (*read == 0)
			{
				co_return unexpected(
					Error::io(IoError::ConnectionReset, "peer closed before the protocol could be identified"));
			}
			bytes.insert(bytes.end(), chunk.begin(), chunk.begin() + static_cast<std::ptrdiff_t>(*read));
		}

		auto classified = bytes;
		co_return Prefix{.bytes = std::move(classified),
		                 .conn = std::make_unique<PrefaceConnection>(std::move(conn), std::move(bytes))};
	}

}  // namespace coroute::net
