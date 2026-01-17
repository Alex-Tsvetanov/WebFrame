#include <boost/asio/detail/call_stack.hpp>
#include <boost/asio/detail/reactor.hpp>
#include <boost/asio/detail/signal_set_service.hpp>
#include <boost/asio/execution_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/local/datagram_protocol.hpp>

#include <boost/asio/detail/strand_executor_service.hpp>
#include <boost/asio/detail/strand_service.hpp>

#include <type_traits>

namespace boost::asio::detail {

///
/// Specialization and definition of some class static variables so that the initializers don't end up
/// in every .cpp file that includes asio headers.
///
/// We do that before including <boost/asio/impl/src.hpp> to avoid compilation errors
/// from asio .ipp files that are referencing these variables.
///
/// This file can cause compiler/linker errors if someone updates boost and the types don't match anymore
/// or the patches were not applied.
///

using CallStackStrandExecutorStack = call_stack<strand_executor_service::strand_impl, unsigned char>;
template<>
tss_ptr<CallStackStrandExecutorStack::context> CallStackStrandExecutorStack::top_{};

using CallStackThreadContextStack = call_stack<thread_context, thread_info_base>;
template<>
tss_ptr<CallStackThreadContextStack::context> CallStackThreadContextStack::top_{};

using StrandServiceStack = call_stack<strand_service::strand_impl, unsigned char>;
template<>
tss_ptr<StrandServiceStack::context> StrandServiceStack::top_{};

template<typename Key, typename Value>
tss_ptr<typename call_stack<Key, Value>::context> call_stack<Key, Value>::top_{};

template<>
posix_global_impl<system_context> posix_global_impl<system_context>::instance_{};

template<typename T>
posix_global_impl<T> posix_global_impl<T>::instance_{};

template<typename Type>
service_id<Type> execution_context_service_base<Type>::id{};

using ResolverService = resolver_service<boost::asio::ip::tcp>;
template<>
service_id<ResolverService> execution_context_service_base<ResolverService>::id{};

using ResolverServiceUdp = resolver_service<boost::asio::ip::udp>;
template<>
service_id<ResolverServiceUdp> execution_context_service_base<ResolverServiceUdp>::id{};

using DeadlineService =
    deadline_timer_service<chrono_time_traits<std::chrono::steady_clock, wait_traits<std::chrono::steady_clock>>>;
template<>
service_id<DeadlineService> execution_context_service_base<DeadlineService>::id{};

using UdpSocketService = reactive_socket_service<ip::udp>;
template<>
service_id<UdpSocketService> execution_context_service_base<UdpSocketService>::id{};

using TcpSocketService = reactive_socket_service<ip::tcp>;
template<>
service_id<TcpSocketService> execution_context_service_base<TcpSocketService>::id{};

using LocalDatagramSocketService = reactive_socket_service<local::datagram_protocol>;
template<>
service_id<LocalDatagramSocketService> execution_context_service_base<LocalDatagramSocketService>::id{};

template<>
service_id<signal_set_service> execution_context_service_base<signal_set_service>::id{};

template<>
service_id<strand_executor_service> execution_context_service_base<strand_executor_service>::id{};

#if defined(BOOST_ASIO_HAS_KQUEUE)
template<>
service_id<boost::asio::detail::kqueue_reactor>
    execution_context_service_base<boost::asio::detail::kqueue_reactor>::id{};
#endif

// Enforcing trivial types to avoid global static initializers. Will catch regressing changes to asio
// (or if someone updated boost and forgot to patch it).
static_assert(std::is_trivial_v<service_id<UdpSocketService>>);
static_assert(std::is_trivial_v<service_id<ResolverService>>);
static_assert(std::is_trivial_v<service_id<ResolverServiceUdp>>);
static_assert(std::is_trivial_v<service_id<DeadlineService>>);
static_assert(std::is_trivial_v<service_id<UdpSocketService>>);
static_assert(std::is_trivial_v<service_id<TcpSocketService>>);
static_assert(std::is_trivial_v<service_id<signal_set_service>>);
#if defined(BOOST_ASIO_HAS_KQUEUE)
static_assert(std::is_trivial_v<service_id<boost::asio::detail::kqueue_reactor>>);
#endif
} // namespace boost::asio::detail

#include <boost/asio/impl/src.hpp>
