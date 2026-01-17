//
//  WatchmanClient.swift
//  BlueSocket
//
//  Created by Simon Corsin on 6/5/19.
//

import Foundation

struct WatchmanFileNotification {
    enum Kind {
        case changed
        case created
        case removed
    }

    let url: URL
    let kind: Kind
}

private struct WatchmanObserver {
    let callback: ([WatchmanFileNotification]) -> Void
    var rootURL: URL?
}

class WatchmanClient: WatchmanConnectionDelegate {

    private let logger: ILogger
    private let subscriptionToken = try! "\"subscription\":".utf8Data()

    private let connection: WatchmanConnection
    private let lock = DispatchSemaphore.newLock()

    private var observerById: [String: WatchmanObserver] = [:]

    init(logger: ILogger) throws {
        self.logger = logger
        do {
            let response = try SyncProcessHandle.run(logger: logger, command: "watchman", arguments: ["get-sockname"])
            let parsedResponse = try WatchmanGetSocknameResponse.fromJSON(try response.utf8Data())
            self.connection = try WatchmanConnection(logger: logger, sockname: parsedResponse.sockname)
            self.connection.delegate = self
        } catch let error {
            throw CompilerError("Failed to start watchman: \(error.legibleLocalizedDescription)")
        }
    }

    private func unsubscribe(path: String, id: String) {
        let logger = self.logger
        _ = self.connection.send(data: try! "[\"unsubscribe\", \"\(path)\", \"\(id)\"]".utf8Data())
            .catch { (error) -> Void in
                logger.error("Failed to unsubscribe path '\(path)': \(error.legibleLocalizedDescription)")
        }
    }

    private func watchProject(path: String) -> Promise<WatchmanWatchProjectResponse> {
        logger.debug("Watchman watching project at path: \(path)")
        let json = try! "[\"watch-project\", \"\(path)\"]".utf8Data()
        return self.connection.send(data: json).then { data -> WatchmanWatchProjectResponse in
            return try WatchmanWatchProjectResponse.fromJSON(data)
        }
    }

    func subscribe(projectPath: String, path: String, onChange: @escaping ([WatchmanFileNotification]) -> Void) -> Cancelable {
        return subscribe(projectPath: projectPath, paths: [path], onChange: onChange)
    }

    func subscribe(projectPath: String, paths: [String], onChange: @escaping ([WatchmanFileNotification]) -> Void) -> Cancelable {
        let id = UUID().uuidString

        lock.lock {
            observerById[id] = WatchmanObserver(callback: onChange, rootURL: nil)
        }

        let cancelObservering = CallbackCancelable {
            self.lock.lock {
                self.observerById.removeValue(forKey: id)
            }
        }

        _ = self.watchProject(path: projectPath).then { response -> Promise<Data> in
            do {
                self.lock.lock {
                    self.observerById[id]?.rootURL = URL(fileURLWithPath: response.watch)
                }

                let dirnameExpressions: [String]

                // response.relativePath returns path relative to the watchRoot
                // null if relativePath is the same as watchRoot
                if let relativePath = response.relativePath {
                    dirnameExpressions = paths.map{watchPath in
                        let concatPath = "\(relativePath)/\(watchPath)"
                        let normalizedPath = (concatPath as NSString).standardizingPath

                        return normalizedPath
                    }
                } else {
                    dirnameExpressions = paths
                }

                let dirNameString = dirnameExpressions.map { #"["dirname", "\#($0)"]"# }.joined(separator: ", ")
                let expressionJson = #"{"expression": ["anyof", \#(dirNameString)]}"#

                let command = "[\"subscribe\", \"\(response.watch)\", \"\(id)\", \(expressionJson)]"

                self.logger.debug("Watchman subscribe: \(command)")

                return self.connection.send(data: try command.utf8Data())
            } catch let error {
                return Promise(error: error)
            }
        }.catch { error -> Void in
            self.logger.error("Failed to observe path '\(projectPath)': \(error.legibleLocalizedDescription)")
            cancelObservering.cancel()
        }

        return CallbackCancelable {
            cancelObservering.cancel()
            self.unsubscribe(path: projectPath, id: id)
        }
    }

    func watchmanConnection(_ watchmanConnection: WatchmanConnection, didReceiveUnilateralPacket packet: Data) {
        do {
            guard packet.range(of: subscriptionToken) != nil else {
                logger.error("Received unrecognized unilateral Watchman packet of size \(packet.count)")
                return
            }
            let response = try WatchmanSubscribeEvent.fromJSON(packet)

            let wrappedObserver = lock.lock {
                observerById[response.subscription]
            }

            guard let observer = wrappedObserver, let rootURL = observer.rootURL else {
                logger.error("Resolved Watchman Universal Packet on an unresolved observer")
                return
            }

            let notifications: [WatchmanFileNotification] = response.files.map {
                let kind: WatchmanFileNotification.Kind
                if !$0.exists {
                    kind = .removed
                } else if $0.new {
                    kind = .created
                } else {
                    kind = .changed
                }
                return WatchmanFileNotification(url: rootURL.resolving(path: $0.name), kind: kind)
            }

            observer.callback(notifications)
        } catch let error {
            logger.error("Unable to parse Watchman subscribe event: \(error.legibleLocalizedDescription)")
        }
    }

}
