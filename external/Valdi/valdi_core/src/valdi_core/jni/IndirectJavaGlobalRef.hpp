//
//  IndirectJavaGlobalRef.hpp
//  valdi_core-android
//
//  Created by Simon Corsin on 2/15/23.
//

#pragma once

#include "valdi_core/cpp/Utils/ReferenceTable.hpp"
#include <djinni/jni/djinni_support.hpp>
#include <utility>
#include <vector>

namespace ValdiAndroid {

/**
 The JNI implementation on Android has a maximum size of 65k for its global reference
 tables. This limit can be reached fairly easily when dealing with a lot of objects.
 IndirectJavaGlobalRef provides a global-ref like abstraction that uses a Java array
 to store the global references. This causes JNI to only hold 1 Java global ref,
 which is the array itself. The trade-off is that each time a jobject reference
 needs to be retrieved from the indirect global reference, an array lookup needs to
 be done, and a lock needs to be acquired.
 */
class IndirectJavaGlobalRef {
public:
    IndirectJavaGlobalRef();
    IndirectJavaGlobalRef(IndirectJavaGlobalRef&& other) noexcept;
    IndirectJavaGlobalRef(const IndirectJavaGlobalRef& other);
    ~IndirectJavaGlobalRef();

    size_t retainCount() const;

    inline bool isNull() const {
        return _id.isNull();
    }

    djinni::LocalRef<jobject> get() const;

    IndirectJavaGlobalRef& operator=(const IndirectJavaGlobalRef& other);
    IndirectJavaGlobalRef& operator=(IndirectJavaGlobalRef&& other) noexcept;

    static IndirectJavaGlobalRef make(jobject object, const char* tag);

    static Valdi::ReferenceTableStats dumpStats();

private:
    Valdi::SequenceID _id;

    IndirectJavaGlobalRef(const Valdi::SequenceID& id);

    void retain();
    void release();
};

} // namespace ValdiAndroid
