//
//  SCValdiBlockHelper.h
//  valdi_core-ios
//
//  Created by Simon Corsin on 1/31/23.
//

#import "valdi_core/SCValdiFieldValue.h"
#import "valdi_core/SCValdiObjCValue.h"
#include "valdi_core/cpp/Utils/InlineContainerAllocator.hpp"
#import "valdi_core/cpp/Utils/Shared.hpp"
#import "valdi_core/cpp/Utils/StringBox.hpp"
#import <Foundation/Foundation.h>

namespace ValdiIOS {

struct ObjCBlockProxy {
    std::string objcTypeEncoding;
    ObjCValue block;
};

/**
 ObjCBlockHelper helps with calling and creating blocks of a specific C type signature.
 The type signature is defined by the argument types and return value of the block.
 The block helper is also used to send Objective-C messages.
 Block helpers are indexed by their type encoding.  Only the C type is considered,
 meaning that a parameter or return value type can only be a pointer (o), an int (i),
 a long (l), a double (d), a bool (b) or void (v).
 For example, a function with type encoding `oob:d` would be representable as
 `double (*)(const void *, const void *, BOOL)` in C. A block helper with this type
 encoding would allow C++ code to:
   - Invoke a C function pointer of type `double (*)(const void *, const void *, BOOL)`
   - Invoke an Objective-C block of type `double^(id, BOOL)` or `double^(const void *, BOOL)`
   - Send an Objective-C message like `-(double)selectorName:(BOOL)argument`

  The difference in supported arguments is because Objective-C blocks are implemented
 as C function pointers with at least one pointer argument, Objective-C method implementations
 are implemented as C function pointers with at least two pointer arguments.
 */
class ObjCBlockHelper : public Valdi::SimpleRefCountable {
public:
    ~ObjCBlockHelper() override;

    size_t getParametersSize() const;

    const SCValdiFieldValueType& getReturnType() const;

    const SCValdiFieldValueType* getParameterTypes() const;

    ObjCValue invokeWithBlock(__unsafe_unretained id block, const ObjCValue* parameters);
    ObjCValue invokeWithImp(IMP imp, const ObjCValue* parameters);

    ObjCValue makeBlock(SCValdiBlockTrampoline blockTrampoline);
    ObjCBlockProxy makeSelectorImpProxy(size_t fieldIndex);

    static Valdi::Ref<ObjCBlockHelper> make(SCValdiFunctionInvoker invoker,
                                            SCValdiBlockFactory factory,
                                            SCValdiFieldValueType returnType,
                                            size_t parametersSize,
                                            const SCValdiFieldValueType* parameterTypes);

    static Valdi::Ref<ObjCBlockHelper> make(const Valdi::StringBox& typeEncoding,
                                            SCValdiFunctionInvoker invoker,
                                            SCValdiBlockFactory factory);

private:
    SCValdiFunctionInvoker _invoker;
    SCValdiBlockFactory _factory;
    SCValdiFieldValueType _returnType;
    size_t _parametersSize;

    friend Valdi::InlineContainerAllocator<ObjCBlockHelper, SCValdiFieldValueType>;

    ObjCBlockHelper(SCValdiFunctionInvoker invoker,
                    SCValdiBlockFactory factory,
                    SCValdiFieldValueType returnType,
                    size_t parametersSize,
                    const SCValdiFieldValueType* parameterTypes);
};

} // namespace ValdiIOS
