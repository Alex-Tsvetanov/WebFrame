//
//  ValdiModuleBuilder.swift
//  Compiler
//
//  Created by Simon Corsin on 3/8/19.
//

import Foundation

private struct LengthField {
    let raw: UInt32
    
    func size() -> Int {
        return Int(raw & (Data.valdiLengthFieldPaddingBit - 1))
    }
    
    func hasPadding() -> Bool {
        return (raw & Data.valdiLengthFieldPaddingBit) != 0
    }
}

extension Parser where T == Data {

    func parseInt() throws -> UInt32 {
        let data = try subsequence(length: 4)

        return data.withUnsafePointer { bufferStart in
            return bufferStart.pointee
        }
    }

    func parseValdiData() throws -> Data {
        let length = LengthField(raw: try parseInt())
        let data = try subsequence(length: length.size())
        if length.hasPadding() {
            let padding = Int(Data.computePadding(size: UInt32(length.size())))
            try advance(distance: padding)
        }
        
        return data
    }
}

class ValdiModuleBuilder {

    private let items: [ZippableItem]
    var compress = true

    init(items: [ZippableItem]) {
        self.items = items
    }

    private func pack() throws -> Data {
        var out = Data()

        let sortedItems = items.sorted { (left, right) -> Bool in
            return left.path > right.path
        }

        for item in sortedItems {
            guard !item.path.hasPrefix("../") else {
                throw CompilerError("Invalid path for entry '\(item.path)'")
            }

            let filename = try item.path.utf8Data()
            let fileData = try item.file.readData()

            out.append(valdiData: filename, padding: true)
            out.append(valdiData: fileData, padding: true)
        }

        return out
    }

    func build() throws -> Data {
        let packetData = try pack()

        var packed = Data()
        packed.append(Magic.valdiMagic)
        packed.append(valdiData: packetData, padding: false)

        if self.compress {
            return try ValdiModuleBuilder.compress(data: packed)
        } else {
            return packed
        }
    }

    static func compress(data: Data) throws -> Data {
        return try ZstdCompressor.compress(data: data)
    }

    static func unpack(module: Data) throws -> [ZippableItem] {
        let moduleData = ZstdCompressor.isZstdCompressed(data: module) ? try ZstdCompressor.decompress(data: module) : module

        let parser = Parser(sequence: moduleData)

        guard try parser.parse(subsequence: Magic.valdiMagic) else {
            throw CompilerError("Did not find valdi magic in module")
        }

        let dataLength = try parser.parseInt()
        if moduleData.count != dataLength + 8 {
            throw CompilerError("Unexpected module length \(moduleData.count - 4), expected \(dataLength)")
        }

        var out = [ZippableItem]()

        while !parser.isAtEnd {
            let filenameData = try parser.parseValdiData()
            let fileData = try parser.parseValdiData()

            guard let filename = String(data: filenameData, encoding: .utf8) else {
                throw CompilerError("Could not extract file name")
            }
            out.append(ZippableItem(file: .data(fileData), path: filename))
        }

        return out
    }
}
