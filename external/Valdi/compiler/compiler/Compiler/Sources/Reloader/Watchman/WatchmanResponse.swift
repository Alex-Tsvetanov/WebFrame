//
//  WatchmanResponse.swift
//  BlueSocket
//
//  Created by Simon Corsin on 6/5/19.
//

import Foundation

extension Decodable {
    static func fromJSON(_ jsonData: Data, keyDecodingStrategy: JSONDecoder.KeyDecodingStrategy = .convertFromSnakeCase) throws -> Self {
        let decoder = JSONDecoder()
        decoder.keyDecodingStrategy = keyDecodingStrategy
        return try decoder.decode(self, from: jsonData)
    }
}

extension Encodable {
    func toJSON(outputFormatting: JSONEncoder.OutputFormatting = [], keyEncodingStrategy: JSONEncoder.KeyEncodingStrategy = .convertToSnakeCase) throws -> Data {
        let encoder = JSONEncoder()
        encoder.outputFormatting = outputFormatting
        encoder.keyEncodingStrategy = keyEncodingStrategy
        return try encoder.encode(self)
    }
}

struct WatchmanGetSocknameResponse: Codable {
    let version: String
    let sockname: String
}

struct WatchmanErrorResponse: Codable {
    let version: String
    let error: String
}

struct WatchmanSubscribeEventFile: Codable {
    let exists: Bool
    let new: Bool
    let name: String
}

struct WatchmanSubscribeEvent: Codable {
    let root: String
    let files: [WatchmanSubscribeEventFile]
    let subscription: String
}

struct WatchmanWatchProjectResponse: Decodable {
    let watcher: String
    let watch: String
    let relativePath: String?
    let version: String
}
