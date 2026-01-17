public protocol ValdiMarshallableObject : ValdiConstructable, ValdiMarshallable {
}

public protocol ValdiMarshallableInterface : ValdiMarshallable {
}

public protocol ValdiMarshallableIntEnum : ValdiMarshallableEnum, RawRepresentable where RawValue == Int32 {
    var rawValue: Int32 { get }
}

public protocol ValdiMarshallableStringEnum : ValdiMarshallableEnum, RawRepresentable where RawValue == String {
    var rawValue: String { get }
}

public protocol ValdiMarshallableEnum : ValdiConstructable, CaseIterable {
}

public protocol ValdiConstructable {
    init(from marshaller: ValdiMarshaller, at objectIndex: Int) throws
}

public protocol ValdiMarshallable : AnyObject {
    func push(to marshaller: ValdiMarshaller) throws -> Int
}

public func allEnumCases<E: CaseIterable & RawRepresentable>(of type: E.Type = E.self) -> [E.RawValue] {
    Array(E.allCases.map {$0.rawValue})
}