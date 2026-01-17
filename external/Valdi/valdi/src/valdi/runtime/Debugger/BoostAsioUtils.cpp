//
//  BoostAsioUtils.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 10/3/19.
//

#include "valdi/runtime/Debugger/BoostAsioUtils.hpp"
#include "utils/debugging/Assert.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"

#include <boost/asio/ip/host_name.hpp>

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <sys/types.h>

namespace boost {
void throw_exception(std::exception const& ex) { // NOLINT(readability-identifier-naming)
    SC_ASSERT_FAIL(ex.what());
    std::abort();
}
} // namespace boost

namespace Valdi {

Error errorFromBoostError(const boost::system::error_code& ec) {
    return Error(StringCache::getGlobal().makeString(ec.message()));
}

std::vector<std::string> getAllAvailableAddresses(boost::asio::io_service& /*ioService*/) {
    std::vector<std::string> allAddresses;

#if !defined(__ANDROID__)
    struct ifaddrs* ifAddrStruct = nullptr;
    struct ifaddrs* ifa = nullptr;

    getifaddrs(&ifAddrStruct);

    if (ifAddrStruct != nullptr) {
        for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr == nullptr) {
                continue;
            }

            if (ifa->ifa_addr->sa_family == AF_INET) {
                // is a valid IP4 Address
                char addressBuffer[INET_ADDRSTRLEN];
                inet_ntop(AF_INET,
                          &(reinterpret_cast<struct sockaddr_in*>(ifa->ifa_addr))->sin_addr,
                          addressBuffer,
                          INET_ADDRSTRLEN);
                allAddresses.emplace_back(std::string_view(addressBuffer));
            }
        }

        freeifaddrs(ifAddrStruct);
    }
#endif

    return allAddresses;
}

} // namespace Valdi
