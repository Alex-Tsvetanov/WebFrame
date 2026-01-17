#include "valdi_core/cpp/Utils/Promise.hpp"
#include "valdi_core/cpp/Utils/ValueMarshaller.hpp"
#include "valdi_core/jni/JNIMethodUtils.hpp"
#include "valdi_core/jni/JavaCache.hpp"
#include "valdi_core/jni/JavaUtils.hpp"
#include "valdi_core/jni/JavaValue.hpp"
#include <fbjni/fbjni.h>

namespace fbjni = facebook::jni;

namespace ValdiAndroid {

class AndroidPromiseCallback final : public Valdi::PromiseCallback {
public:
    AndroidPromiseCallback(const Valdi::Ref<Valdi::ValueMarshaller<JavaValue>>& valueMarshaller, jobject completion)
        : _valueMarshaller(valueMarshaller), _completion(JavaEnv(), completion, "Promise Completion") {}

    ~AndroidPromiseCallback() final = default;

    void onSuccess(const Valdi::Value& value) final {
        Valdi::SimpleExceptionTracker exceptionTracker;
        auto unmarshalledValue = _valueMarshaller->unmarshall(value, Valdi::ReferenceInfoBuilder(), exceptionTracker);
        if (!exceptionTracker) {
            JavaCache::get().getPromiseCallbackOnFailureMethod().call(
                _completion.toObject(), toJavaThrowable(JavaEnv(), exceptionTracker.extractError()));
        } else {
            if (unmarshalledValue.isNull()) {
                if (_valueMarshaller->isOptional()) {
                    JavaCache::get().getPromiseCallbackOnSuccessMethod().call(_completion.toObject(),
                                                                              JavaObject(JavaEnv()));
                } else {
                    JavaCache::get().getPromiseCallbackOnSuccessMethod().call(_completion.toObject(),
                                                                              JavaCache::getUnitValueClassInstance());
                }
            } else {
                JavaCache::get().getPromiseCallbackOnSuccessMethod().call(
                    _completion.toObject(), JavaObject(JavaEnv(), unmarshalledValue.getObject()));
            }
        }
    }

    void onFailure(const Valdi::Error& error) final {
        JavaCache::get().getPromiseCallbackOnFailureMethod().call(_completion.toObject(),
                                                                  toJavaThrowable(JavaEnv(), error));
    }

private:
    Valdi::Ref<Valdi::ValueMarshaller<JavaValue>> _valueMarshaller;
    GlobalRefJavaObjectBase _completion;
};

class CppPromiseJNI : public fbjni::JavaClass<CppPromiseJNI> {
public:
    static constexpr auto kJavaDescriptor = "Lcom/snap/valdi/promise/CppPromise;";

    static Valdi::Ref<Valdi::Promise> getPromise(jlong ptr) {
        return bridge<Valdi::Promise>(ptr);
    }

    static Valdi::Ref<Valdi::ValueMarshaller<JavaValue>> getValueMarshaller(jlong ptr) {
        return bridge<Valdi::ValueMarshaller<JavaValue>>(ptr);
    }

    // NOLINTNEXTLINE
    static void nativeOnComplete(fbjni::alias_ref<fbjni::JClass> /* clazz */,
                                 jlong ptr,
                                 jlong valueMarshallerNativeHandle,
                                 jobject completion) {
        auto promise = getPromise(ptr);
        auto valueMarshaller = getValueMarshaller(valueMarshallerNativeHandle);
        if (promise == nullptr || valueMarshaller == nullptr) {
            return;
        }

        promise->onComplete(Valdi::makeShared<AndroidPromiseCallback>(valueMarshaller, completion));
    }

    // NOLINTNEXTLINE
    static void nativeCancel(fbjni::alias_ref<fbjni::JClass> /* clazz */, jlong ptr) {
        auto promise = getPromise(ptr);
        if (promise != nullptr) {
            promise->cancel();
        }
    }

    // NOLINTNEXTLINE
    static jboolean nativeIsCancelable(fbjni::alias_ref<fbjni::JClass> /* clazz */, jlong ptr) {
        auto promise = getPromise(ptr);
        if (promise == nullptr) {
            return static_cast<jboolean>(false);
        }
        return static_cast<jboolean>(promise->isCancelable());
    }

    static void registerNatives() {
        javaClassStatic()->registerNatives({
            makeNativeMethod("nativeOnComplete", CppPromiseJNI::nativeOnComplete),
            makeNativeMethod("nativeCancel", CppPromiseJNI::nativeCancel),
            makeNativeMethod("nativeIsCancelable", CppPromiseJNI::nativeIsCancelable),
        });
    }
};

} // namespace ValdiAndroid
