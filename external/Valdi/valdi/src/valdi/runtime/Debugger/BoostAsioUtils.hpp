//
//  BoostAsioUtils.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 10/3/19.
//

#pragma once

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshorten-64-to-32"

#include <boost/asio/buffer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/write.hpp>

#pragma clang diagnostic pop

#include "valdi_core/cpp/Utils/Error.hpp"
#include <string>

namespace Valdi {

Error errorFromBoostError(const boost::system::error_code& ec);
std::vector<std::string> getAllAvailableAddresses(boost::asio::io_service& ioService);

} // namespace Valdi
