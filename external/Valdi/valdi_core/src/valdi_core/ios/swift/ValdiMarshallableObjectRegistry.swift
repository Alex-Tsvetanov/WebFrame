import Foundation
import valdi_core
import ValdiCoreCPP

open class ValdiMarshallableObjectRegistry {
    public static let shared = ValdiMarshallableObjectRegistry()

    public var registryCpp: SwiftValdiMarshallableObjectRegistryRef
    public var registeredClasses: [String] = []
    private init() {
        // import the ObjC ValueSchemaRegistry so that registered ObjC Classes are available
        let objCValueSchemaRegistry = SCValdiMarshallableObjectRegistryGetSharedInstance().getValueSchemaRegistryPtr()
        self.registryCpp = SwiftValdiMarshallableObjectRegistry_Create(objCValueSchemaRegistry)
    }

    deinit {
        SwiftValdiMarshallableObjectRegistry_Destroy(self.registryCpp)
    }

    /// Register a class with the registry
    /// - Parameters: cls: The class to register
    /// - Parameters: objectDescriptor: The object descriptor for the class
    public func register(cls : AnyClass, objectDescriptor: ValdiMarshallableObjectDescriptor) {
        register(className: NSStringFromClass(cls), objectDescriptor: objectDescriptor)
    }

    /// Register a class with the registry
    /// - Parameters: className: The className to register
    /// - Parameters: objectDescriptor: The object descriptor for the class
    public func register(className : String, objectDescriptor: ValdiMarshallableObjectDescriptor) {
        if registeredClasses.contains(className) {
            return
        }
        registeredClasses.append(className)
        objectDescriptor.identifiers.forEach { $0.registerFunc?() }
        className.withCString { classNameC in
            SwiftValdiMarshallableObjectRegistry_RegisterClass(
                self.registryCpp,
                classNameC,
                objectDescriptor.getOrCreateCDescriptor()
            )
        }
    }

    public func registerObjCClass(className: String) {
        guard let cls = NSClassFromString(className) as? SCValdiMarshallableObject.Type else {
            return
        }
        SCValdiMarshallableObjectRegistryGetSharedInstance().forceLoad(cls)
    }

    public func registerUntypedObjCClass(className: String) {
        guard let cls = NSClassFromString(className) else {
            return
        }
        SCValdiMarshallableObjectRegistryGetSharedInstance().register(cls, objectDescriptor: SCValdiMarshallableObjectDescriptorMake(nil, nil, nil, SCValdiMarshallableObjectTypeUntyped))
    }

    /// Register a string enum with the registry
    /// - Parameters: enumName: Name of the enum to register
    /// - Parameters: enumCases: Array of the enum cases as strings
    public func registerStringEnum(enumName : String, enumCases: [String]) {
        // Build a C-style array from enumCases with a null terminator
        var cStringArray = enumCases.map { $0.toCString() }
        cStringArray.append(nil)

        let cEnumCaseArray = cStringArray.toCArray()
        defer {
            var cEnumCasesItr = cEnumCaseArray
            while cEnumCasesItr?.pointee != nil {
                deallocateCString(cEnumCasesItr?.pointee)
                cEnumCasesItr = cEnumCasesItr?.advanced(by: 1)
            }
            deallocateCArray(cEnumCaseArray)
        }

        enumName.withCString { enumNameC in
            SwiftValdiMarshallableObjectRegistry_RegisterStringEnum(
                self.registryCpp,
                enumNameC,
                cEnumCaseArray,
                Int32(enumCases.count)
            )
        }
    }

    /// Register a int enum with the registry
    /// - Parameters: enumName: Name of the enum to register
    /// - Parameters: enumCases: Array of the enum cases as Int32s 
    public func registerIntEnum(enumName : String, enumCases: [Int32]) {
        let cEnumCaseArray = enumCases.toCArray()
        defer {
            deallocateCArray(cEnumCaseArray)
        }

        enumName.withCString { enumNameC in
            SwiftValdiMarshallableObjectRegistry_RegisterIntEnum(
                self.registryCpp,
                enumNameC,
                cEnumCaseArray,
                Int32(enumCases.count)
            )
        }
    }

    /// Set the active scheme for a class in a marshaller
    /// - Parameters: className: The class name. Must be registered with the registry.
    /// - Parameters: marshaller: The marshaller to set the scheme in
    public func setActiveSchemeOfClassInMarshaller(className: String, marshaller: ValdiMarshaller) {
        className.withCString { classNameC in
            SwiftValdiMarshallableObjectRegistry_SetActiveSchemeOfClassInMarshaller(
                self.registryCpp,
                classNameC,
                marshaller.marshallerCpp
            )
        }
    }
}
