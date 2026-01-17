#include <iostream>
#include <ostream>
#include <stdlib.h>
#include <string.h>

#include <map>
#include <string>

#include "valdi/runtime/Interfaces/IJavaScriptContext.hpp"
#include "valdi/runtime/JavaScript/JSPropertyNameIndex.hpp"
#include "valdi/runtime/JavaScript/JSValueRefHolder.hpp"
#include "valdi/runtime/JavaScript/JavaScriptTaskScheduler.hpp"
#include "valdi/runtime/JavaScript/JavaScriptUtils.hpp"
#include "valdi/runtime/JavaScript/WrappedJSValueRef.hpp"
#include "valdi/runtime/Utils/HexUtils.hpp"
#include "valdi/v8/V8JavaScriptContextFactory.hpp"
#include "valdi_core/cpp/Utils/ByteBuffer.hpp"
#include "valdi_core/cpp/Utils/ConsoleLogger.hpp"
#include "valdi_core/cpp/Utils/StaticString.hpp"

#include "utils/platform/TargetPlatform.hpp"
#include "valdi/runtime/Interfaces/IJavaScriptBridge.hpp"
#include "valdi/runtime/JavaScript/JSFunctionWithCallable.hpp"
#include "valdi/runtime/JavaScript/JavaScriptFunctionCallContext.hpp"
#include "valdi_core/cpp/Utils/ByteBuffer.hpp"
#include "valdi_core/cpp/Utils/StaticString.hpp"
#include <future>

using namespace Valdi;

struct V8PropertyNamesVisitor : public IJavaScriptPropertyNamesVisitor {
    bool visitPropertyName(IJavaScriptContext& context,
                           JSValue object,
                           const JSPropertyName& propertyName,
                           JSExceptionTracker& exceptionTracker) override {
        (void)object;
        (void)exceptionTracker;
        std::cout << context.propertyNameToString(propertyName) << std::endl;
        return true;
    }
};

int main() {
    auto factory = Valdi::V8::V8JavaScriptContextFactory();
    auto ctx = factory.createJsContext(nullptr, Valdi::ConsoleLogger::getLogger());

    {
        Valdi::JSExceptionTracker tracker(*ctx);
        auto val = ctx->evaluate("\"hello\"", "test.js", tracker);
        auto valUnpacked = ctx->valueToBool(val.get(), tracker);
        printf("value: %d\n", static_cast<int>(valUnpacked));
    }

    {
        Valdi::JSExceptionTracker tracker(*ctx);

        std::vector<double> expectedFloatArray = {0, 1, 2, 3};
        auto buffer = makeShared<ByteBuffer>(
            reinterpret_cast<const Byte*>(expectedFloatArray.data()),
            reinterpret_cast<const Byte*>(expectedFloatArray.data() + expectedFloatArray.size()));
        auto float64Data = Value(makeShared<ValueTypedArray>(Float64Array, buffer->toBytesView()));
        auto typedArray =
            newTypedArrayFromBytesView(*ctx, Float64Array, float64Data.getTypedArray()->getBuffer(), tracker);
        {
            auto value = ctx->getObjectPropertyForIndex(typedArray.get(), 3, tracker);
            auto valUnpacked = ctx->valueToDouble(value.get(), tracker);
            printf("value: %f\n", valUnpacked);
        }
        ctx->setObjectPropertyIndex(typedArray.get(), 3, ctx->newNumber(1337.9).get(), tracker);
        {
            auto value = ctx->getObjectPropertyForIndex(typedArray.get(), 3, tracker);
            auto valUnpacked = ctx->valueToDouble(value.get(), tracker);
            printf("value: %f\n", valUnpacked);
        }
    }
    {
        auto key = ctx->newPropertyName("test");
        auto pullprop = ctx->propertyNameToString(key.get());
        std::cout << pullprop.toStringView() << std::endl;
    }
    {
        Valdi::JSExceptionTracker tracker(*ctx);

        auto obj = ctx->newObject(tracker);
        if (!tracker) {
            printf("Failed to create an object\n");
            return 1;
        }
        auto val = ctx->newNumber(1337.3);
        ctx->setObjectProperty(obj.get(), "test", val.get(), tracker);
        ctx->setObjectProperty(obj.get(), "test2", val.get(), false, tracker);
        ctx->setObjectProperty(obj.get(), "test3", val.get(), true, tracker);

        if (!tracker) {
            printf("Failed to set an object property\n");
            return 1;
        }
        auto retval = ctx->getObjectProperty(obj.get(), "test", tracker);
        if (!tracker) {
            printf("Failed to get an object property\n");
            return 1;
        }

        {
            auto test = ctx->getObjectProperty(obj.get(), "testB", tracker);
            printf("type is undefined: %d\n", static_cast<int>(ctx->isValueUndefined(test.get())));
        }

        auto retvalUnpacked = ctx->valueToDouble(retval.get(), tracker);
        printf("value: %f\n", retvalUnpacked);
        printf("visit: \n");
        V8PropertyNamesVisitor visitor;
        ctx->visitObjectPropertyNames(obj.get(), tracker, visitor);
    }

    // { // Too large Q.Q
    //     printf("heap: \n");
    //     auto heapDump = ctx->dumpHeap();
    //     std::cout << heapDump.asStringView() << std::endl;
    // }
    {
        Valdi::JSExceptionTracker tracker(*ctx);
        auto functionName = STRING_LITERAL("MyFunctionName");
        auto function =
            ctx->newFunction(makeShared<JSFunctionWithCallable>(ReferenceInfoBuilder().withObject(functionName),
                                                                [](auto& callContext) -> Valdi::JSValueRef {
                                                                    return callContext.getContext().newUndefined();
                                                                }),
                             tracker);
    }
    return 0;
}
