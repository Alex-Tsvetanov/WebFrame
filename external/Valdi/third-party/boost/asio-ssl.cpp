#include <boost/asio/detail/call_stack.hpp>
#include <boost/asio/execution_context.hpp>
#include <boost/asio/detail/deadline_timer_service.hpp>

#include <type_traits>

namespace boost::asio::detail {
template<typename Key, typename Value>
tss_ptr<typename call_stack<Key, Value>::context>
call_stack<Key, Value>::top_;

template<typename T>
posix_global_impl<T> posix_global_impl<T>::instance_;

using DeadlineServicePosix = deadline_timer_service<time_traits<boost::posix_time::ptime>>;
template<>
service_id<DeadlineServicePosix> execution_context_service_base<DeadlineServicePosix>::id{};

// Enforcing trivial types to avoid global static initializers. Will catch regressing changes to asio
// (or if someone updated boost and forgot to patch it).
static_assert(std::is_trivial_v<service_id<DeadlineServicePosix>>);
}

#include <boost/asio/ssl/impl/src.hpp>
