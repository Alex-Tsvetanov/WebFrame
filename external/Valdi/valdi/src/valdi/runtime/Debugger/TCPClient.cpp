//
//  TCPClient.cpp
//  valdi-pc
//
//  Created by Simon Corsin on 10/14/20.
//

#include "valdi/runtime/Debugger/TCPClient.hpp"
#include "valdi/runtime/Debugger/BoostAsioUtils.hpp"
#include "valdi/runtime/Debugger/TCPConnectionImpl.hpp"
#include "valdi_core/cpp/Threading/Thread.hpp"
#include "valdi_core/cpp/Utils/Mutex.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"

namespace Valdi {

class TCPClientImpl;

class TCPClientDisconnectListenerImpl : public SharedPtrRefCountable, public ITCPConnectionDisconnectListener {
public:
    TCPClientDisconnectListenerImpl(const Ref<TCPClientImpl>& client, const Shared<ITCPClientListener>& listener)
        : _client(client), _listener(listener) {}

    ~TCPClientDisconnectListenerImpl() override = default;

    void onDisconnected(const Ref<ITCPConnection>& /*connection*/, const Error& error) override {
        _listener->onDisconnected(error);
        _client = nullptr;
    }

private:
    Ref<TCPClientImpl> _client;
    Shared<ITCPClientListener> _listener;
};

class TCPClientImpl : public SharedPtrRefCountable {
public:
    TCPClientImpl() = default;

    ~TCPClientImpl() override {
        stopServices();
    }

    void connect(const std::string& address, int32_t port, const Shared<ITCPClientListener>& listener) {
        auto strongSelf = strongRef(this);
        auto connection = makeShared<TCPConnectionImpl>(_ioService);
        connection->getSocket().async_connect(
            boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(address), static_cast<uint16_t>(port)),
            [strongSelf, connection, listener](const boost::system::error_code& errorCode) {
                if (errorCode.failed()) {
                    strongSelf->stopServices();
                    listener->onDisconnected(errorFromBoostError(errorCode));
                } else {
                    connection->setDisconnectListener(
                        makeShared<TCPClientDisconnectListenerImpl>(strongSelf, listener).toShared());
                    connection->onReady();
                    listener->onConnected(connection);
                }
            });

        auto result = startServices();
        if (!result) {
            listener->onDisconnected(result.error());
            return;
        }
    }

    void stopServices() {
        _started = false;
        _ioService.stop();

        Ref<Thread> asioThread;
        {
            std::lock_guard<Mutex> lock(_mutex);
            asioThread = std::move(_asioThread);
        }

        if (asioThread != nullptr) {
            asioThread->join();
        }
    }

private:
    Mutex _mutex;
    boost::asio::io_service _ioService;
    Ref<Thread> _asioThread;
    std::atomic_bool _started = {false};

    Result<Void> startServices() {
        std::lock_guard<Mutex> lock(_mutex);

        if (_asioThread != nullptr) {
            return Void();
        }

        _started = true;
        auto threadResult =
            Thread::create(STRING_LITERAL("TCP Client"), ThreadQoSClassNormal, [this]() { this->runIOService(); });
        if (!threadResult) {
            return threadResult.moveError();
        }
        _asioThread = threadResult.moveValue();
        return Void();
    }

    void runIOService() {
        boost::system::error_code ec;
        _ioService.run(ec);
    }
};

TCPClient::TCPClient() : _impl(makeShared<TCPClientImpl>()) {}

TCPClient::~TCPClient() {
    disconnect();
}

void TCPClient::connect(const std::string& address, int32_t port, const Shared<ITCPClientListener>& listener) {
    _impl->connect(address, port, listener);
}

void TCPClient::disconnect() {
    _impl->stopServices();
}

} // namespace Valdi
