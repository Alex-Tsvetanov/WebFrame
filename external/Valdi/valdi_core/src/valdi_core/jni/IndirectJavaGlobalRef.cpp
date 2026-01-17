//
//  IndirectJavaGlobalRef.cpp
//  valdi_core-android
//
//  Created by Simon Corsin on 2/15/23.
//

#include "valdi_core/jni/IndirectJavaGlobalRef.hpp"
#include "utils/debugging/Assert.hpp"
#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include "valdi_core/jni/JavaEnv.hpp"
#include "valdi_core/jni/JavaUtils.hpp"
#include <algorithm>
#include <fmt/format.h>
#include <shared_mutex>

namespace ValdiAndroid {

constexpr size_t kReferencesArrayMinSize = 8096;

/**
 The IndirectJavaGlobalRefTable stores Java references in a Java array.
 It uses a sequence generator that provides 64 bits integer ids that
 have a salt and an index (see Valdi::SequenceIDGenerator class for more
 details). The Java array only grows when all its slots are full.
 */
class IndirectJavaGlobalRefTable {
public:
    IndirectJavaGlobalRefTable() {
        auto jClass =
            JavaEnv::accessEnvRetRef([&](JNIEnv& jniEnv) -> jclass { return jniEnv.FindClass("java/lang/Object"); });
        _objectClass = JavaEnv::newGlobalRef(jClass.get());
    }

    ~IndirectJavaGlobalRefTable() {
        SC_ABORT("IndirectJavaGlobalRefTable should not be destructed");
    }

    Valdi::SequenceID make(jobject object, const char* tag) {
        auto write = _table.writeAccess();
        auto entry = write.makeRef(tag);

        auto index = entry.getIndex();
        if (index >= _referencesArraySize) {
            reallocateReferencesArray(index);
        }

        JavaObjectArray referencesArray(reinterpret_cast<jobjectArray>(_references.get()));
        referencesArray.setObject(index, object);

        return entry.id;
    }

    void retain(Valdi::SequenceID id) {
        _table.writeAccess().retainRef(id);
    }

    void release(Valdi::SequenceID id) {
        auto write = _table.writeAccess();
        if (!write.releaseRef(id)) {
            JavaObjectArray referencesArray(reinterpret_cast<jobjectArray>(_references.get()));
            referencesArray.setObject(static_cast<size_t>(id.getIndex()), nullptr);
        }
    }

    size_t getRetainCount(Valdi::SequenceID id) const {
        return static_cast<size_t>(_table.readAccess().getEntry(id).retainCount);
    }

    djinni::LocalRef<jobject> newLocalRef(Valdi::SequenceID id) const {
        auto read = _table.readAccess();
        auto index = read.getEntry(id).getIndex();

        JavaObjectArray referencesArray(reinterpret_cast<jobjectArray>(_references.get()));
        return referencesArray.getObject(index);
    }

    Valdi::ReferenceTableStats dumpStats() const {
        return _table.dumpStats();
    }

    static IndirectJavaGlobalRefTable& get() {
        static auto* kInstance = new IndirectJavaGlobalRefTable();
        return *kInstance;
    }

private:
    Valdi::ReferenceTable _table;
    size_t _referencesArraySize = 0;
    djinni::GlobalRef<jobject> _references;
    djinni::GlobalRef<jclass> _objectClass;

    void reallocateReferencesArray(size_t minSize) {
        auto previousReferences = std::move(_references);
        auto previousSize = _referencesArraySize;
        auto newSize = std::max(minSize, std::max(kReferencesArrayMinSize, previousSize * 2));

        auto references = JavaEnv::accessEnvRetRef(
            [&](JNIEnv& env) { return env.NewObjectArray(static_cast<jsize>(newSize), _objectClass.get(), nullptr); });

        JavaObjectArray newArray(references.get());
        JavaObjectArray oldArray(reinterpret_cast<jobjectArray>(previousReferences.get()));

        for (size_t i = 0; i < previousSize; i++) {
            auto reference = oldArray.getObject(i);
            newArray.setObject(i, reference.get());
        }

        _references = JavaEnv::newGlobalRef(references.get());
        _referencesArraySize = newSize;
    }
};

IndirectJavaGlobalRef::IndirectJavaGlobalRef(const Valdi::SequenceID& id) : _id(id) {}

IndirectJavaGlobalRef::IndirectJavaGlobalRef() = default;

IndirectJavaGlobalRef::IndirectJavaGlobalRef(IndirectJavaGlobalRef&& other) noexcept : _id(other._id) {
    other._id = Valdi::SequenceID();
}

IndirectJavaGlobalRef::IndirectJavaGlobalRef(const IndirectJavaGlobalRef& other) : _id(other._id) {
    retain();
}

IndirectJavaGlobalRef::~IndirectJavaGlobalRef() {
    release();
}

size_t IndirectJavaGlobalRef::retainCount() const {
    if (isNull()) {
        return 0;
    }

    return IndirectJavaGlobalRefTable::get().getRetainCount(_id);
}

void IndirectJavaGlobalRef::retain() {
    if (!isNull()) {
        IndirectJavaGlobalRefTable::get().retain(_id);
    }
}

void IndirectJavaGlobalRef::release() {
    if (!isNull()) {
        IndirectJavaGlobalRefTable::get().release(_id);
        _id = Valdi::SequenceID();
    }
}

djinni::LocalRef<jobject> IndirectJavaGlobalRef::get() const {
    if (isNull()) {
        return djinni::LocalRef<jobject>();
    }

    return IndirectJavaGlobalRefTable::get().newLocalRef(_id);
}

IndirectJavaGlobalRef& IndirectJavaGlobalRef::operator=(const IndirectJavaGlobalRef& other) {
    if (this != &other) {
        release();

        _id = other._id;

        retain();
    }

    return *this;
}

IndirectJavaGlobalRef& IndirectJavaGlobalRef::operator=(IndirectJavaGlobalRef&& other) noexcept {
    if (this != &other) {
        release();

        _id = other._id;
        other._id = Valdi::SequenceID();
    }

    return *this;
}

IndirectJavaGlobalRef IndirectJavaGlobalRef::make(jobject object, const char* tag) {
    if (object == nullptr) {
        return IndirectJavaGlobalRef();
    }

    auto id = IndirectJavaGlobalRefTable::get().make(object, tag);

    return IndirectJavaGlobalRef(id);
}

Valdi::ReferenceTableStats IndirectJavaGlobalRef::dumpStats() {
    return IndirectJavaGlobalRefTable::get().dumpStats();
}

} // namespace ValdiAndroid
