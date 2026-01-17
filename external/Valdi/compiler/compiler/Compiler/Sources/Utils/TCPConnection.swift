//
//  SocketConnection.swift
//  BlueSocket
//
//  Created by Simon Corsin on 6/5/19.
//

import Foundation
import BlueSocket

enum TCPConnectionProcessDataResult {
    case valid(dataLength: Int)
    case invalid(dataToDiscard: Int)
    case notEnoughData(lengthNeeded: Int)
}

protocol TCPConnectionDataProcessor {
    func process(data: Data) -> TCPConnectionProcessDataResult
}

protocol TCPConnectionDelegate: AnyObject {
    func connection(_ connection: TCPConnection, didReceiveData data: Data)
    func connection(_ connection: TCPConnection, didDisconnectWithError error: Error?)
}

protocol TCPConnection: AnyObject {
    var delegate: TCPConnectionDelegate? { get set }
    var dataProcessor: TCPConnectionDataProcessor? { get set }
    var key: String { get }

    func startListening()
    func send(data: Data)
    func close()
}

extension TCPConnection {
    func processBuffer(_ buffer: inout Data) -> Int? {
        guard let dataProcessor = self.dataProcessor else {
            let data = buffer
            buffer = Data()
            delegate?.connection(self, didReceiveData: data)
            return nil
        }

        while !buffer.isEmpty {
            let result = dataProcessor.process(data: buffer)
            switch result {
            case .valid(let dataLength):
                let outData: Data
                if dataLength == buffer.count {
                    outData = buffer
                    buffer = Data()
                } else {
                    outData = buffer[0..<dataLength]
                    buffer = Data(buffer.advanced(by: dataLength))
                }

                delegate?.connection(self, didReceiveData: outData)
            case .invalid(let dataToDiscard):
                buffer = buffer.advanced(by: dataToDiscard)
            case let .notEnoughData(lengthNeeded):
                return lengthNeeded
            }
        }
        return nil
    }
}
