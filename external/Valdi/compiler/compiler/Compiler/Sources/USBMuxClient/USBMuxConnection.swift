//
//  USBMuxConnection.swift
//  Compiler
//
//  Created by saniul on 09/10/2019.
//

import Foundation
import BlueSocket

enum USBMuxResponseCode: UInt32 {
    case ok = 0
    case badCommand = 1
    case badDevice = 2
    case connectionRefused = 3
    // ??? = 4
    // ??? = 5
    case badVersion = 6
}

enum USBMuxRequestType: String {
    case listen = "Listen"
    case connect = "Connect"
}

struct USBMuxDeviceId: Hashable, CustomStringConvertible {
    let deviceId: Int

    init(_ deviceId: Int) {
        self.deviceId = deviceId
    }

    var description: String {
        return deviceId.description
    }
}

struct USBMuxDeviceInfo {
    let deviceId: USBMuxDeviceId
    let properties: [String: Any]
}

class USBMuxConnection {
    var channel: DispatchIO?
    var broadcastHandler: ((_ packet: USBMuxPlistPayload) -> Void)?

    private let logger: ILogger
    private let queue: DispatchQueue
    var socket: Socket?
    private var nextPacketTag: USBMuxPacketTag = 0
    private var waitingCallbacks: [USBMuxPacketTag: ((Result<USBMuxPlistPayload, Error>) -> Void)] = [:]
    private var isReadingPackets: Bool = false
    private var shouldAutoReadPackets: Bool { broadcastHandler != nil }

    init(logger: ILogger, queue: DispatchQueue) {
        self.logger = logger
        self.queue = queue
    }

    deinit {
        channel?.close()
    }

    func open(onEnd: @escaping (Result<Void, Error>) -> Void) throws {
        guard channel == nil else {
            fatalError("Channel should not be open when calling open()")
        }

        guard let socketSig = try Socket.Signature(socketType: .stream, proto: .unix, path: "/var/run/usbmuxd") else {
            throw NSError(domain: NSPOSIXErrorDomain, code: Int(errno), userInfo: nil)
        }

        let socket = try Socket.create(connectedUsing: socketSig)
        self.socket = socket
        guard socket.isConnected else {
            throw NSError(domain: NSPOSIXErrorDomain, code: Int(errno), userInfo: nil)
        }

        channel = DispatchIO(type: .stream, fileDescriptor: socket.socketfd, queue: queue, cleanupHandler: { [socket] (errno: Int32) in
            // Intentional retain, need to clean up the channel before we clean up the socket
            _ = socket

            if errno == 0 {
                onEnd(.success(()))
            } else {
                let error = NSError(domain: NSPOSIXErrorDomain, code: Int(errno), userInfo: nil)
                onEnd(.failure(USBMuxError.ioFailure(error)))
            }
        })
    }

    func sendRequest(_ payload: USBMuxPlistPayload, callback: @escaping (Result<Void, Error>) -> Void) {
        nextPacketTag += 1
        let tag = nextPacketTag

        sendPacketWithPayload(payload, tag: tag, callback: { [weak self] result in
            self?.logger.verbose("[USBMuxConnection] Sent: error=\(String(describing: result.failure)) payload=\(payload) packetTag=\(tag)")

            switch result {
            case let .failure(error):
                callback(.failure(error))
            case .success:
                self?.waitingCallbacks[tag] = { result in
                    guard let this = self else { return }
                    let transformedResult = result.flatMap(this.checkResponsePayload)
                    callback(transformedResult)
                }
            }
        })

        if !isReadingPackets {
            scheduleReadPacket()
        }
    }

    func scheduleReadPacket() {
        assert(!isReadingPackets)

        scheduleReadPacketWithCallback { [weak self] (packetTag, result) in
            guard let this = self else { return }

            this.logger.verbose("[USBMuxConnection] Received: error=\(String(describing: result.failure)) packet=\(String(describing: result.success)) packetTag=\(packetTag)")
            if packetTag == 0 {
                // Broadcast message
                switch result {
                case let .success(packet):
                    this.broadcastHandler?(packet)
                case let .failure(error):
                    this.logger.error("Packet read failure: \(error.legibleLocalizedDescription)")
                }
            } else {
                // Response
                if let requestCallback = this.waitingCallbacks[packetTag] {
                    this.waitingCallbacks[packetTag] = nil
                    requestCallback(result)
                } else {
                    this.logger.warn("[USBMuxConnection] Warning: Ignoring response packet for which there is no registered callback. error=\(String(describing: result.failure)) packet=\(String(describing: result.success))")
                }
            }

            if this.shouldAutoReadPackets {
                this.scheduleReadPacket()
            }
        }
    }

    // MARK: Private

    private func sendDispatchData(data: DispatchData, callback: @escaping (Result<Void, Error>) -> Void) {
        guard let channel = channel else {
            fatalError("Channel should exist by now")
        }

        channel.write(offset: 0, data: data, queue: queue, ioHandler: { (done: Bool, _: DispatchData?, errno: Int32) in
            guard done else {
                return
            }

            if errno == 0 {
                callback(.success(()))
            } else {
                callback(.failure(NSError(domain: NSPOSIXErrorDomain, code: Int(errno), userInfo: nil)))
            }
        })
    }

    private func sendData(data: Data, callback: @escaping (Result<Void, Error>) -> Void) {
        // retaining the passed-in data using a block capture to avoid having DispatchData
        // copy the bytes
        var tmp: Data? = data
        let dispatchData: DispatchData = data.withUnsafeBytes { bytes in
            return DispatchData(bytesNoCopy: bytes, deallocator: .custom(queue, {
                _ = tmp
                tmp = nil
            }))
        }
        sendDispatchData(data: dispatchData, callback: callback)
    }

    private func readFromOffset(offset: off_t, length: size_t, callback: @escaping (Result<DispatchData, Error>) -> Void) {
        guard let channel = channel else {
            fatalError("Channel should exist by now")
        }

        channel.read(offset: offset, length: length, queue: queue) { (done: Bool, data: DispatchData?, errno: Int32) in
            guard done else {
                return
            }

            guard let data = data else {
                callback(.failure(USBMuxError.readEmptyData))
                return
            }

            if errno == 0 {
                callback(.success(data))
            } else {
                let error = NSError(domain: NSPOSIXErrorDomain, code: Int(errno), userInfo: nil)
                callback(.failure(USBMuxError.ioFailure(error)))
            }
        }
    }

    private func checkResponsePayload(_ payload: USBMuxPlistPayload) -> Result<Void, Error> {
        guard let responseNumber = payload["Number"] as? Int else {
            return .failure(USBMuxError.unknown)
        }

        guard responseNumber != 0 else {
            return .success(())
        }

        guard let responseCode = USBMuxResponseCode(rawValue: UInt32(responseNumber)) else {
            return .failure(USBMuxError.unknownResponse(responseCode: UInt32(responseNumber)))
        }

        return .failure(USBMuxError.failedResponse(responseCode))
    }

    private func sendPacketOfType(_ type: USBMuxPacketType, overProtocol protocol: USBMuxPacketProtocol, tag: UInt32, payload: Data, callback: @escaping (Result<Void, Error>) -> Void) {
        let upacket = USBMuxPacket(protocol: `protocol`, type: type, tag: tag, payload: payload)
        let data = upacket.fullPacketData.withUnsafeBytes { bytes in
            DispatchData(bytes: bytes)
        }
        sendDispatchData(data: data, callback: callback)
    }

    private func sendPacketWithPayload(_ payload: USBMuxPlistPayload, tag: UInt32, callback: @escaping (Result<Void, Error>) -> Void) {
        do {
            let payloadData = try PropertyListSerialization.data(fromPropertyList: payload, format: .xml, options: .zero)
            sendPacketOfType(.plistPayload, overProtocol: .plist, tag: tag, payload: payloadData, callback: callback)
        } catch {
            callback(.failure(error))
        }
    }

    private func scheduleReadPacketWithCallback(callback: @escaping (_ packetTag: UInt32, Result<USBMuxPlistPayload, Error>) -> Void) {
        isReadingPackets = true

        scheduleReadPacketSize(callback: callback)
    }

    private func scheduleReadPacketSize(callback: @escaping (_ packetTag: UInt32, Result<USBMuxPlistPayload, Error>) -> Void) {
        guard let channel = channel else {
            fatalError("Channel should exist by now")
        }

        channel.read(offset: 0,
                     length: Int(USBMuxPacket.fullSizeLayoutSize),
                     queue: queue,
                     ioHandler: { [weak self] (done: Bool, dispatchData: DispatchData?, errno: Int32) in
                        self?.handleReadPacketSize(done: done, dispatchData: dispatchData, errno: errno, callback: callback)
        })
    }

    private func handleReadPacketSize(done: Bool, dispatchData: DispatchData?, errno: Int32, callback: @escaping (_ packetTag: UInt32, Result<USBMuxPlistPayload, Error>) -> Void) {
        guard done else { return }

        guard errno == 0 else {
            isReadingPackets = false
            let error = NSError(domain: NSPOSIXErrorDomain, code: Int(errno), userInfo: nil)
            callback(0, .failure(USBMuxError.ioFailure(error)))
            return
        }

        guard let dispatchData = dispatchData else {
            callback(0, .failure(USBMuxError.readEmptyData))
            return
        }

        let fullPacketSize: UInt32 = dispatchData.withUnsafeBytes(body: { (pointer: UnsafePointer<UInt32>) in
            return pointer.pointee
        })
        let metaAndPayloadSize = fullPacketSize - USBMuxPacket.fullSizeLayoutSize
        let offset = USBMuxPacket.fullSizeLayoutSize

        scheduleReadPacketBody(offset: off_t(offset),
                                    metaAndPayloadSize: Int(metaAndPayloadSize),
                                    callback: callback)
    }

    private func scheduleReadPacketBody(offset: off_t, metaAndPayloadSize: Int, callback: @escaping (_ packetTag: UInt32, Result<USBMuxPlistPayload, Error>) -> Void) {
        guard let channel = channel else {
            fatalError("Channel should exist by now")
        }

        channel.read(offset: off_t(offset),
                     length: Int(metaAndPayloadSize),
                     queue: queue,
                     ioHandler: { [weak self] (done: Bool, dispatchData: DispatchData?, errno: Int32) in
                        self?.handleReadPacketBody(done: done, dispatchData: dispatchData, errno: errno, callback: callback)
        })
    }

    private func handleReadPacketBody(done: Bool, dispatchData: DispatchData?, errno: Int32, callback: @escaping (_ packetTag: UInt32, Result<USBMuxPlistPayload, Error>) -> Void) {
        guard done else { return }

        isReadingPackets = false

         guard errno == 0 else {
             let error = NSError(domain: NSPOSIXErrorDomain, code: Int(errno), userInfo: nil)
             callback(0, .failure(USBMuxError.ioFailure(error)))
             return
         }

         guard let dispatchData = dispatchData else {
             callback(0, .failure(USBMuxError.readEmptyData))
             return
         }

         let data = Data(dispatchData)
         guard let packet = USBMuxPacket(metaAndPayloadData: data) else {
             callback(0, .failure(USBMuxError.couldntParsePacket))
             return
         }

         guard case .plist = packet.protocol else {
             callback(packet.tag, .failure(USBMuxError.unexpectedPacketProtocol))
             return
         }

         guard case .plistPayload = packet.type else {
             callback(packet.tag, .failure(USBMuxError.unexpectedPacketType))
             return
         }

         do {
             let plistResult = try PropertyListSerialization.propertyList(from: packet.payload, options: [], format: nil)
             guard let dict = plistResult as? USBMuxPlistPayload else {
                 callback(packet.tag, .failure(USBMuxError.invalidPlistPayload))
                 return
             }
             callback(packet.tag, .success(dict))
         } catch {
             callback(packet.tag, .failure(error))
             return
         }
    }
}
