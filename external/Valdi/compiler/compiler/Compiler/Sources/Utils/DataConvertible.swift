import Foundation

// Protocol and extensions for easily converting Data to numeric types and back
protocol DataConvertible {
    init?(data: Data)
    var data: Data { get }
}

extension DataConvertible where Self: ExpressibleByIntegerLiteral {

    init?(data: Data) {
        var value: Self = 0
        guard data.count == MemoryLayout.size(ofValue: value) else { return nil }
        _ = withUnsafeMutableBytes(of: &value, { data.copyBytes(to: $0)})
        self = value
    }

    var data: Data {
        return withUnsafeBytes(of: self) { Data($0) }
    }
}

extension Int: DataConvertible { }
extension UInt32: DataConvertible { }
extension Float: DataConvertible { }
extension Double: DataConvertible { }
