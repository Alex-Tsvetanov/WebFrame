//
//  TCPSocketModuleFactory.cpp
//  valdi-pc
//
//  Created by Simon Corsin on 10/14/20.
//

#include "valdi/runtime/JavaScript/Modules/TCPSocketModuleFactory.hpp"
#include "valdi/runtime/Debugger/TCPClient.hpp"
#include "valdi/runtime/Debugger/TCPServer.hpp"

#include "valdi_core/cpp/Utils/Format.hpp"
#include "valdi_core/cpp/Utils/Mutex.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"
#include "valdi_core/cpp/Utils/ValueArrayBuilder.hpp"
#include "valdi_core/cpp/Utils/ValueFunctionWithCallable.hpp"
#include "valdi_core/cpp/Utils/ValueTypedArray.hpp"

#include "valdi_core/cpp/Resources/ValdiPacket.hpp"

namespace Valdi {

enum JSTCPConnectionProtocol {
    JSTCPConnectionProtocolRaw,
    JSTCPConnectionProtocolRawString,
    JSTCPConnectionProtocolValdiBytes,
    JSTCPConnectionProtocolValdiString,
};

class JSTCPConnectionDataListener : public SharedPtrRefCountable, public ITCPConnectionDataListener {
public:
    JSTCPConnectionDataListener(int32_t clientId,
                                const Ref<ValueFunction>& onDataReceived,
                                JSTCPConnectionProtocol protocol)
        : _clientId(clientId), _onDataReceived(onDataReceived), _protocol(protocol) {}

    ~JSTCPConnectionDataListener() override = default;

    void onDataReceived(const Ref<ITCPConnection>& connection, const BytesView& bytes) override {
        if (_protocol == JSTCPConnectionProtocolValdiString || _protocol == JSTCPConnectionProtocolValdiBytes) {
            _packetStream.write(bytes);

            for (;;) {
                auto result = _packetStream.read();
                if (!result) {
                    if (result.error() != ValdiPacket::incompletePacketError()) {
                        // Something bad happened, exiting...
                        connection->close(result.error());
                    }
                    return;
                } else {
                    notifyDataReceived(result.value());
                }
            }
        } else {
            notifyDataReceived(bytes);
        }
    }

    constexpr int32_t getClientId() const {
        return _clientId;
    }

private:
    int32_t _clientId;
    Ref<ValueFunction> _onDataReceived;
    JSTCPConnectionProtocol _protocol;
    ValdiPacketStream _packetStream;

    void notifyDataReceived(const BytesView& bytes) {
        if (_onDataReceived != nullptr) {
            Value value;
            if (_protocol == JSTCPConnectionProtocolValdiString || _protocol == JSTCPConnectionProtocolRawString) {
                value = Value(StringCache::getGlobal().makeString(bytes.asStringView()));
            } else {
                value = Value(makeShared<ValueTypedArray>(Uint8Array, bytes));
            }

            (*_onDataReceived)({Value(_clientId), std::move(value)});
        }
    }
};

class JSTTCPConnectionAutoCloseSentinel : public ValdiObject {
public:
    explicit JSTTCPConnectionAutoCloseSentinel(const Ref<ITCPConnection>& connection) : _connection(connection) {}
    ~JSTTCPConnectionAutoCloseSentinel() override {
        _connection->close(Error("Autoclosing since nothing is referencing the connection object"));
    }

    VALDI_CLASS_HEADER_IMPL(JSTTCPConnectionAutoCloseSentinel)

private:
    Ref<ITCPConnection> _connection;
};

static Value makeJsConnection(const Ref<ITCPConnection>& connection, JSTCPConnectionProtocol protocol) {
    Value out;
    out.setMapValue(
        "send", Value(makeShared<ValueFunctionWithCallable>([=](const ValueFunctionCallContext& callContext) -> Value {
            BytesView bytes;
            auto value = callContext.getParameter(0);
            if (value.isTypedArray()) {
                bytes = value.getTypedArray()->getBuffer();
            } else if (value.isString()) {
                auto valueStr = value.toStringBox();
                bytes = BytesView(
                    valueStr.getInternedString(), reinterpret_cast<const Byte*>(valueStr.getCStr()), valueStr.length());
            } else {
                callContext.getExceptionTracker().onError(Error("Data parameter should be bytes or string"));
                return Value::undefined();
            }

            if (protocol == JSTCPConnectionProtocolValdiString || protocol == JSTCPConnectionProtocolValdiBytes) {
                auto out = makeShared<ByteBuffer>();
                ValdiPacket::write(bytes.data(), bytes.size(), *out);
                bytes = out->toBytesView();
            }

            connection->submitData(bytes);

            return Value::undefined();
        })));

    out.setMapValue(
        "close", Value(makeShared<ValueFunctionWithCallable>([=](const ValueFunctionCallContext& callContext) -> Value {
            connection->close(Error(callContext.getParameter(0).toStringBox()));
            return Value::undefined();
        })));

    out.setMapValue("address", Value(StringCache::getGlobal().makeString(connection->getAddress())));
    out.setMapValue("$autoCloseSentinel", Value(makeShared<JSTTCPConnectionAutoCloseSentinel>(connection)));

    return out;
}

class JSTCPListener : public ValdiObject, public ITCPServerListener, public ITCPClientListener {
public:
    JSTCPListener(const Ref<ValueFunction>& onClientConnected,
                  const Ref<ValueFunction>& onClientDisconnected,
                  const Ref<ValueFunction>& onDataReceived,
                  JSTCPConnectionProtocol protocol)
        : _onClientConnected(onClientConnected),
          _onClientDisconnected(onClientDisconnected),
          _onDataReceived(onDataReceived),
          _protocol(protocol) {}

    ~JSTCPListener() override {
        // Making sure the server is stopped if the user failed to do it
        auto server = _server.lock();
        if (server != nullptr) {
            server->stop();
        }
    }

    // TCP client impl
    void onConnected(const Ref<ITCPConnection>& connection) override {
        connection->setDataListener(makeShared<JSTCPConnectionDataListener>(0, _onDataReceived, _protocol).toShared());

        if (_onClientConnected != nullptr) {
            (*_onClientConnected)({makeJsConnection(connection, _protocol)});
        }
    }

    void onDisconnected(const Error& error) override {
        if (_onClientDisconnected != nullptr) {
            (*_onClientDisconnected)({Value(error.toString())});
        }
    }

    // TCP Server impl
    void onClientConnected(const Ref<ITCPConnection>& client) override {
        std::lock_guard<Mutex> lock(_mutex);

        auto clientId = ++_clientIdSequence;

        client->setDataListener(
            makeShared<JSTCPConnectionDataListener>(clientId, _onDataReceived, _protocol).toShared());

        if (_onClientConnected != nullptr) {
            (*_onClientConnected)({Value(clientId), makeJsConnection(client, _protocol)});
        }
    }

    void onClientDisconnected(const Ref<ITCPConnection>& client, const Error& error) override {
        if (_onClientDisconnected != nullptr) {
            auto clientId = castOrNull<JSTCPConnectionDataListener>(client->getDataListener())->getClientId();

            (*_onClientDisconnected)({Value(clientId), Value(error.toString())});
        }
    }

    void setTCPServer(const Ref<ITCPServer>& server) {
        _server = server.toWeak();
    }

    VALDI_CLASS_HEADER_IMPL(JSTCPListener)

private:
    Ref<ValueFunction> _onClientConnected;
    Ref<ValueFunction> _onClientDisconnected;
    Ref<ValueFunction> _onDataReceived;
    mutable Mutex _mutex;
    int _clientIdSequence = 0;
    JSTCPConnectionProtocol _protocol;
    Weak<ITCPServer> _server;
};

TCPSocketModuleFactory::TCPSocketModuleFactory() = default;
TCPSocketModuleFactory::~TCPSocketModuleFactory() = default;

static Result<Ref<JSTCPListener>> makeJSTCPListener(const Value& protocol, const Value& listeners) {
    JSTCPConnectionProtocol resolvedProtocol;
    auto protocolString = protocol.toStringBox();
    if (protocolString == "raw") {
        resolvedProtocol = JSTCPConnectionProtocolRaw;
    } else if (protocolString == "string") {
        resolvedProtocol = JSTCPConnectionProtocolRawString;
    } else if (protocolString == "valdi-bytes") {
        resolvedProtocol = JSTCPConnectionProtocolValdiBytes;
    } else if (protocolString == "valdi-string") {
        resolvedProtocol = JSTCPConnectionProtocolValdiString;
    } else {
        return Error(STRING_FORMAT("Unknown protocol '{}'", protocolString));
    }

    auto onConnect = listeners.getMapValue("onConnected").getFunctionRef();
    auto onDisconnect = listeners.getMapValue("onDisconnected").getFunctionRef();
    auto onDataReceived = listeners.getMapValue("onDataReceived").getFunctionRef();

    return makeShared<JSTCPListener>(onConnect, onDisconnect, onDataReceived, resolvedProtocol);
}

StringBox TCPSocketModuleFactory::getModulePath() {
    return STRING_LITERAL("TCPSocket");
}

Value TCPSocketModuleFactory::loadModule() {
    Value out;

    out.setMapValue(
        "createServer",
        Value(makeShared<ValueFunctionWithCallable>([](const ValueFunctionCallContext& callContext) -> Value {
            auto port = callContext.getParameter(0).toInt();
            auto listenerResult = makeJSTCPListener(callContext.getParameter(1), callContext.getParameter(2));
            if (!listenerResult) {
                callContext.getExceptionTracker().onError(listenerResult.error());
                return Value::undefined();
            }
            auto listener = listenerResult.moveValue();
            auto server = TCPServer::create(port, listener.get());
            listener->setTCPServer(server);

            Value out;
            out.setMapValue("listener", Value(listener));
            out.setMapValue(
                "start",
                Value(makeShared<ValueFunctionWithCallable>([=](const ValueFunctionCallContext& callContext) -> Value {
                    auto success = server->start();
                    if (!success) {
                        callContext.getExceptionTracker().onError(success.moveError());
                    }

                    return Value::undefined();
                })));
            out.setMapValue("stop",
                            Value(makeShared<ValueFunctionWithCallable>(
                                [=](const ValueFunctionCallContext& /*callContext*/) -> Value {
                                    server->stop();
                                    return Value::undefined();
                                })));
            out.setMapValue("getPort",
                            Value(makeShared<ValueFunctionWithCallable>(
                                [=](const ValueFunctionCallContext& /*callContext*/) -> Value {
                                    auto port = server->getBoundPort();
                                    return Value(static_cast<int32_t>(port));
                                })));
            out.setMapValue("getAddresses",
                            Value(makeShared<ValueFunctionWithCallable>(
                                [=](const ValueFunctionCallContext& /*callContext*/) -> Value {
                                    ValueArrayBuilder out;
                                    for (auto& address : server->getAvailableAddresses()) {
                                        out.emplace(StringCache::getGlobal().makeString(std::move(address)));
                                    }
                                    return Value(out.build());
                                })));

            return out;
        })));
    out.setMapValue(
        "createConnection",
        Value(makeShared<ValueFunctionWithCallable>([](const ValueFunctionCallContext& callContext) -> Value {
            auto address = callContext.getParameter(0).toStringBox();
            auto port = callContext.getParameter(1).toInt();
            auto listenerResult = makeJSTCPListener(callContext.getParameter(2), callContext.getParameter(3));
            if (!listenerResult) {
                callContext.getExceptionTracker().onError(listenerResult.moveError());
                return Value::undefined();
            }

            auto client = makeShared<TCPClient>();
            client->connect(address.slowToString(), port, listenerResult.value().toShared());

            Value out;
            out.setMapValue("cancel",
                            Value(makeShared<ValueFunctionWithCallable>(
                                [=](const ValueFunctionCallContext& /*callContext*/) -> Value {
                                    client->disconnect();
                                    return Value::undefined();
                                })));

            return out;
        })));

    return out;
}

} // namespace Valdi
