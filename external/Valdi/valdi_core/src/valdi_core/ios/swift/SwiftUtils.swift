import ValdiCoreCPP

@_cdecl("swiftCallWithMarshaller")
public func swiftCallWithMarshaller(_ valdiFunction: UnsafeMutableRawPointer, _ marshaller: UnsafeMutableRawPointer) -> Bool {
    let function = Unmanaged<ValdiFunction>.fromOpaque(valdiFunction).takeUnretainedValue()
    let valdiMarshaller = ValdiMarshaller(marshaller)
    return function.perform(with: valdiMarshaller, sync: true)
}

@_cdecl("retainSwiftObject")
public func retainSwiftObject(_ swiftObject: UnsafeMutableRawPointer) {
    _ = Unmanaged<AnyObject>.fromOpaque(swiftObject).retain()
}

@_cdecl("releaseSwiftObject")
public func releaseSwiftObject(_ swiftObject: UnsafeMutableRawPointer) {
    Unmanaged<AnyObject>.fromOpaque(swiftObject).release()
}
