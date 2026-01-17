//
//  ValdiDaemonProtocolConnection.swift
//  Compiler
//
//  Created by Simon Corsin on 2/15/19.
//  Copyright © 2019 Snap Inc. All rights reserved.
//

import Foundation
import SwiftProtobuf

protocol ValdiDaemonProtocolConnectionDelegate: AnyObject {

    func connection(_ connection: ValdiDaemonProtocolConnection, didDisconnectWithError error: Error?)

    func connection(_ connection: ValdiDaemonProtocolConnection, didReceiveRequest request: DaemonClientRequest, responsePromise: Promise<DaemonServerResponse>)

    func connection(_ connection: ValdiDaemonProtocolConnection, didReceiveEvent event: DaemonClientEvent)

}

final class ValdiDaemonProtocolConnection {
    var connectionKey: String { return connection.key }

    // Sometimes (i.e. when using ADB) we may have an established connection, but it will fail as soon as we read from
    // it, since the stream is actually empty. isConfirmed will be true if we've read at least a single packet.
    var isConfirmed: Bool { return connection.everReadPacket }

    private let logger: ILogger
    private weak var delegate: ValdiDaemonProtocolConnectionDelegate?

    private let connection: DaemonTCPConnection
    private let queue = DispatchQueue(label: "com.snap.valdi.DaemonService.client")

    private var promiseByRequestId = [String: Promise<DaemonClientResponse>]()

    init(logger: ILogger, connection: DaemonTCPConnection, delegate: ValdiDaemonProtocolConnectionDelegate) {
        self.logger = logger
        self.delegate = delegate
        self.connection = connection
        connection.delegate = self
        connection.startListening()
    }

    func close() {
        queue.async {
            self.connection.close()
        }
    }

    func send(event: DaemonServerEvent) {
        queue.async {
            var payload = DaemonServerPayload()
            payload.event = event
            self.doSend(payload: payload)
        }
    }

    func send(request: DaemonServerRequest) -> Promise<DaemonClientResponse> {
        let requestId = UUID().uuidString

        let promise = Promise<DaemonClientResponse>()

        queue.async {
            self.promiseByRequestId[requestId] = promise

            var payload = DaemonServerPayload()
            payload.request = request
            payload.request?.request_id = requestId

            self.doSend(payload: payload)
        }

        return promise
    }

    private func doSend(payload: DaemonServerPayload) {
        do {
            let encoder = JSONEncoder()
            encoder.outputFormatting = .sortedKeys
            let data = try encoder.encode(payload)
            connection.send(data: data)
        } catch let error {
            logger.error("Failed to send data to client: \(error)")
        }
    }

    private func processRequestResponse(_ response: Result<DaemonServerResponse, Error>, requestId: String) {
        queue.async {
            var payload = DaemonServerPayload()

            do {
                payload.response = try response.get()
            } catch let error {
                payload.response = DaemonServerResponse()
                payload.response?.error = DaemonErrorResponse(error_message: error.legibleLocalizedDescription)
            }
            payload.response?.request_id = requestId

            self.doSend(payload: payload)
        }
    }

    private func processPacket(data: Data) {
        do {
            let decoder = JSONDecoder()
            let payload = try decoder.decode(DaemonClientPayload.self, from: data)

            if let request = payload.request {

                let promise = Promise<DaemonServerResponse>()
                let requestId = request.request_id
                promise.onComplete { [weak self] (result) in
                    self?.processRequestResponse(result, requestId: requestId)
                }

                delegate?.connection(self, didReceiveRequest: request, responsePromise: promise)
                return
            }

            if let response = payload.response {

                guard let pendingPromise = promiseByRequestId[response.request_id] else {
                    logger.error("Received a response without an associated request, ignoring (request id: \(response.request_id))")
                    return
                }

                if let error = response.error {
                    pendingPromise.reject(error: CompilerError(error.error_message))
                } else {
                    pendingPromise.resolve(data: response)
                }
                return
            }

            if let event = payload.event {
                delegate?.connection(self, didReceiveEvent: event)
                return
            }

            // If we make it this far, something is wrong
            throw CompilerError("Unknown data format: \(payload)")
        } catch let error {
            logger.error("Failed to deserialize packet: \(error.legibleLocalizedDescription)")
        }
    }

}

extension ValdiDaemonProtocolConnection: DaemonTCPConnectionDelegate {

    func connection(_ connection: DaemonTCPConnection, didReceiveData data: Data) {
        queue.async {
            self.processPacket(data: data)
        }
    }

    func connection(_ connection: DaemonTCPConnection, didDisconnectWithError error: Error?) {
        queue.async {
            self.delegate?.connection(self, didDisconnectWithError: error)
        }
    }

}
