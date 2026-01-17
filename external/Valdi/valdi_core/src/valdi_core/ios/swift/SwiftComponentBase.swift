import valdi_core

open class SwiftComponentBase: SCValdiRootView {
    public init(viewModel:(any ValdiMarshallableObject)? = nil,
                context:(any ValdiMarshallableObject)? = nil,
                runtime: SCValdiRuntimeProtocol) throws {
        let marshaller: ValdiMarshaller = ValdiMarshaller()
        if (viewModel != nil) {
            _ = try marshaller.push(viewModel!)
        } else {
            _ = try marshaller.pushUntypedMap([:])
        }
        if (context != nil) {
            _ = try marshaller.push(context!)
        } else {
            _ = try marshaller.pushUntypedMap([:])
        }
        super.init(owner: nil, cppMarshaller:marshaller.marshallerCpp, runtime: runtime)
    }
}
