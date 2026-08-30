#include "coroute/net/io_context.hpp"

#include <utility>

namespace coroute::net
{

	Task<WriteResult> Connection::async_write_zero_copy(const void* buffer, size_t len)
	{
		(void)buffer;
		(void)len;
		co_return unexpected(Error::io(IoError::InvalidArgument,
		                               "zero-copy send is not supported by this backend"));
	}

	bool Connection::supports_zero_copy_send() const noexcept { return false; }


	namespace
	{

		// Parameters rather than captures: a coroutine copies its parameters into the
		// frame, so the handler outlives the loop that started this. A capturing
		// lambda's closure object would not.
		Task<void> accept_loop(IoContext* context, Listener* target, ConnectionHandler on_connection)
		{
			while (!context->stopped())
			{
				auto conn = co_await target->async_accept();
				if (!conn)
				{
					// A failed accept is usually one bad client rather than a dead
					// listener, so keep accepting unless we are shutting down.
					if (context->stopped())
					{
						break;
					}
					continue;
				}
				on_connection(std::move(*conn));
			}
		}

	}  // namespace

	void start_accept_pool(IoContext& ctx, Listener& listener, const ConnectionHandler& handler, size_t depth)
	{
		if (depth == 0)
		{
			depth = 1;
		}

		// Each coroutine gets its own copy of the handler, which is why this takes a
		// const reference rather than a value: a by-value parameter would add one
		// copy on top of the per-coroutine ones.
		for (size_t i = 0; i < depth; ++i)
		{
			accept_loop(&ctx, &listener, handler).start_detached();
		}
	}

}  // namespace coroute::net
