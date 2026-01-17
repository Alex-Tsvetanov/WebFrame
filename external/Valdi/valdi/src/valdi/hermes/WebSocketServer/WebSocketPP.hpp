//
//  WebSocketPP.hpp
//  valdi
//
//  Created by Edward Lee on 02/20/2024.
//

#pragma once

#ifdef VALDI_HAS_WEBSOCKET_SERVER

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#pragma clang diagnostic ignored "-Wnull-pointer-subtraction"

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#pragma clang diagnostic pop

using WebSocketPPServer = websocketpp::server<websocketpp::config::asio>;

#endif // VALDI_HAS_WEBSOCKET_SERVER
