//
//  SCValdiMarshallableObjectDescriptor.h
//  valdi-ios
//
//  Created by Simon Corsin on 6/3/20.
//

#import "valdi_core/SCMacros.h"
#import "valdi_core/SCValdiFieldValue.h"
#import <Foundation/NSObjCRuntime.h>

SC_EXTERN_C_BEGIN

/**
 A descriptor for a single field of a marshallable object
 */
typedef struct SCValdiMarshallableObjectFieldDescriptor {
    /*
     The name of the property when marshalled.
     */
    const char* name;

    /**
     The name of selector to get the property.
     If not set, the name will be used as the selector name.
     */
    const char* selName;

    /**
     The encoded type descriptor.
     Supported types:

     - o: Object. 'o' should be followed by the class
          name of the object and terminated by @.
          Example: 'oSCMyObject@'

     - d: A double

     - b: A boolean

     - a: An array. 'a' should be followed by the item
          type.
          Example: 'ab': Array of booleans

     - s: A string

     - f: A function. 'f' should be followed by '(', the
          list of parameter types separated by ',', a ')'
          to end the parameter list, and the return type.
          A block invoker/factory index can be provided with
          a ':' right before the parameters declaration.
          Examples:
         'f(d,b)v': Function taking a double and bool parameter
                    and returning void.
         'f:1(b)b': Function taking a bool and returning a bool.
                    The block invoker/facotry is at index 1.

     - v: Void. Only supported as a return type for functions.

     - m: Map.

     - z: Byte buffer.

     - u: Untyped object.

     - v: any object.

     - e: Enum.

     - g: Generic object. 'g' should be followed by '<', the list
          of type argument type encodings separated by ',', a'>'
          to end the type argument list, and then the class name of
          the object and terminated by @.

     Each type can be directly followed by '?' to make them optionals.
     */
    const char* type;

} SCValdiMarshallableObjectFieldDescriptor;

/**
 Provides a way for the runtime to invoke a function with a signature
 that isn't natively supported in the runtime, and to create a block
 with that same signature.
 */
typedef struct SCValdiMarshallableObjectBlockSupport {
    const char* typeEncoding;
    SCValdiFunctionInvoker invoker;
    SCValdiBlockFactory factory;
} SCValdiMarshallableObjectBlockSupport;

typedef enum : uint8_t {
    SCValdiMarshallableObjectTypeClass,
    SCValdiMarshallableObjectTypeProtocol,
    SCValdiMarshallableObjectTypeFunction,
    // TODO(simon): Will likely remove to just keep protocol
    // This is used temporarily to avoid the $nativeInstance
    // boxing for those types
    SCValdiMarshallableObjectTypeUntyped,
} SCValdiMarshallableObjectType;

typedef struct SCValdiMarshallableObjectDescriptor {
    const SCValdiMarshallableObjectFieldDescriptor* fields;
    const char* const* identifiers;
    const SCValdiMarshallableObjectBlockSupport* blocks;
    SCValdiMarshallableObjectType objectType;
} SCValdiMarshallableObjectDescriptor;

extern SCValdiMarshallableObjectDescriptor SCValdiMarshallableObjectDescriptorMake(
    const SCValdiMarshallableObjectFieldDescriptor* fields,
    const char* const* identifiers,
    const SCValdiMarshallableObjectBlockSupport* blocks,
    SCValdiMarshallableObjectType objectType);

SC_EXTERN_C_END
