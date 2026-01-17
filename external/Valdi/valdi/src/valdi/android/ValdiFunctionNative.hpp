#include "valdi_core/jni/JNIMethodUtils.hpp"
#include "valdi_core/jni/JavaUtils.hpp"
#include <fbjni/fbjni.h>

#include "valdi_core/cpp/Utils/ValueFunction.hpp"

namespace fbjni = facebook::jni;

namespace ValdiAndroid {

class ValdiFunctionNative : public fbjni::JavaClass<ValdiFunctionNative> {
public:
    static constexpr auto kJavaDescriptor = "Lcom/snap/valdi/callable/ValdiFunctionNative;";

    //  NOLINTNEXTLINE
    static jboolean nativePerform(fbjni::alias_ref<fbjni::JClass> /* clazz */,
                                  jlong ptr,
                                  jint flags,
                                  jlong marshallerPtr) {
        auto* env = fbjni::Environment::current();
        auto* value = reinterpret_cast<Valdi::Value*>(ptr);
        if (value == nullptr) {
            ValdiAndroid::throwJavaValdiException(env, "Cannot call native function after it has been destroyed");
            return static_cast<jboolean>(false);
        }

        const auto& function = value->getFunction();
        auto* marshaller = ValdiAndroid::unwrapMarshaller(env, marshallerPtr);
        if (marshaller == nullptr) {
            return static_cast<jboolean>(false);
        }
        auto retValue = (*function)(Valdi::ValueFunctionCallContext(static_cast<Valdi::ValueFunctionFlags>(flags),
                                                                    marshaller->getValues(),
                                                                    marshaller->size(),
                                                                    marshaller->getExceptionTracker()));
        if (!marshaller->getExceptionTracker()) {
            ValdiAndroid::throwJavaValdiException(env, marshaller->getExceptionTracker().extractError());
            return static_cast<jboolean>(false);
        }

        marshaller->push(std::move(retValue));
        return static_cast<jboolean>(true);
    }

    static void registerNatives() {
        javaClassStatic()->registerNatives({
            makeNativeMethod("nativePerform", ValdiFunctionNative::nativePerform),
        });
    }
};

} // namespace ValdiAndroid
