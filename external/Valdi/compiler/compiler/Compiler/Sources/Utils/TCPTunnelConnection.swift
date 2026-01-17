//
//  TCPTunnelConnection.swift
//  Compiler
//
//  Created by elee2 on 02/22/2024.
//

import Foundation

struct TCPTunnelEndpoint {
    let tcpConnection: TCPConnection
    var started: Bool = false
}
protocol TCPTunnelConnectionDelegate: AnyObject {
    func disconnected(_ tunnel: TCPTunnelConnection)
}

class TCPTunnelConnection {
    let key: String
    private var endpoints: [TCPTunnelEndpoint] = []
    private let queue = DispatchQueue(label: "TCPTunnelConnection")

    weak var delegate: TCPTunnelConnectionDelegate?

    init(a: TCPConnection, b: TCPConnection) {
        self.key = a.key + "<->" + b.key
        self.endpoints = [TCPTunnelEndpoint(tcpConnection: a), TCPTunnelEndpoint(tcpConnection: b)]
    }

    deinit {
        for endpoint in self.endpoints{
            endpoint.tcpConnection.close()
        }
    }

    func start () {
        for index in self.endpoints.indices {
            self.endpoints[index].started = true
            self.endpoints[index].tcpConnection.delegate = self
            self.endpoints[index].tcpConnection.startListening()
        }
    }

    func stop () {
        queue.async { [weak self] in
            guard let self = self else { return }
            for endpoint in self.endpoints{
                endpoint.tcpConnection.close()
            }
        }
    }
}

extension TCPTunnelConnection: TCPConnectionDelegate {
    func connection(_ connection: TCPConnection, didReceiveData data: Data) {
        queue.async { [weak self] in
            guard let self = self else { return }
            let otherEndpoint = self.endpoints.first { $0.tcpConnection !== connection }
            otherEndpoint?.tcpConnection.send(data: data)
        }
    }

    func connection(_ connection: TCPConnection, didDisconnectWithError error: Error?) {
        queue.async { [weak self] in
            guard let self = self else { return }

            guard let thisEndpointIndex = self.endpoints.firstIndex(where: { $0.tcpConnection === connection }),
                let otherEndpointIndex = self.endpoints.firstIndex(where: { $0.tcpConnection !== connection}) else {
                return
            }

            if self.endpoints[thisEndpointIndex].started {
                self.endpoints[thisEndpointIndex].started = false
                if self.endpoints[otherEndpointIndex].started {
                    self.endpoints[otherEndpointIndex].tcpConnection.close()
                    self.delegate?.disconnected(self)
                }
            }
        }
    }
}
