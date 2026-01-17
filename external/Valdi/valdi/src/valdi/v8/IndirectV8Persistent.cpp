//
//  IndirectV8Persistent.cpp
//  valdi_core-android
//
//  Created by Simon Corsin on 2/15/23.
//

#include "IndirectV8Persistent.hpp"
#include "utils/base/NonCopyable.hpp"
#include "utils/debugging/Assert.hpp"
#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include <algorithm>
#include <cstdio>
#include <fmt/format.h>
#include <shared_mutex>

namespace Valdi::V8 {
// TODO(rjaber): The locking is unnecessary we should move towards a reference table with zero locks
constexpr size_t kReferencesArrayMinSize = 8096;

class IndirectV8PersistentTable;

class V8ManagedPointerWrapper : public snap::NonCopyable {
public:
    V8ManagedPointerWrapper(void* opaque, const std::function<void(void*)>* deleter, SequenceID sequenceID)
        : _opaque(opaque), _deleter(*deleter), _sequenceID(sequenceID) {};

    V8ManagedPointerWrapper(V8ManagedPointerWrapper&&) = delete;
    V8ManagedPointerWrapper& operator=(V8ManagedPointerWrapper&&) = delete;

    void release();

private:
    void* _opaque;
    std::function<void(void*)> _deleter;
    SequenceID _sequenceID;
};

class IndirectV8PersistentTable : public snap::NonCopyable {
public:
    IndirectV8PersistentTable() = default;

    IndirectV8PersistentTable(IndirectV8PersistentTable&&) = delete;
    IndirectV8PersistentTable& operator=(IndirectV8PersistentTable&&) = delete;

    ~IndirectV8PersistentTable() {
        SC_ABORT("IndirectV8PersistentTable should not be destructed");
    }

    SequenceID make(v8::Isolate* isolate,
                    const v8::Local<v8::Value>& value,
                    void* opaque,
                    const std::function<void(void*)>* weak_deleter,
                    const char* tag) {
        SC_ASSERT((opaque == nullptr && weak_deleter == nullptr) || // NOLINT
                  (opaque != nullptr && weak_deleter != nullptr));  // NOLINT
        auto write = _table.writeAccess();
        auto entry = write.makeRef(tag);

        auto index = entry.getIndex();
        if (index >= _referenceArray.capacity()) {
            auto newSize = std::max(index, std::max(kReferencesArrayMinSize, _referenceArray.capacity() * 2));
            _referenceArray.reserve(newSize);
        }

        _referenceArray[index].Reset(isolate, value);
        _referenceArray[index].SetWrapperClassId(kWrappedObjectID);
        if (weak_deleter != nullptr && opaque != nullptr) {
            auto parameter = new V8ManagedPointerWrapper(opaque, weak_deleter, entry.id);
            _referenceArray[index].SetWeak(
                parameter,
                [](const v8::WeakCallbackInfo<V8ManagedPointerWrapper>& info) {
                    info.GetParameter()->release();
                    delete info.GetParameter();
                },
                v8::WeakCallbackType::kParameter);
            _referenceArray[index].SetWrapperClassId(kWrappedObjectID);
        } else {
            _referenceArray[index].ClearWeak();
            _referenceArray[index].SetWrapperClassId(kNoWrapperID);
        }
        return entry.id;
    }

    bool contains(SequenceID id) {
        return _table.readAccess().contains(id);
    }

    void retain(SequenceID id) {
        _table.writeAccess().retainRef(id);
    }

    void release(SequenceID id) {
        auto write = _table.writeAccess();
        if (!write.releaseRef(id)) {
            _referenceArray[id.getIndex()].Reset();
        }
    }

    size_t getRetainCount(SequenceID id) const {
        return static_cast<size_t>(_table.readAccess().getEntry(id).retainCount);
    }

    std::pair<v8::Local<v8::Value>, uint16_t> newLocalRef(v8::Isolate* isolate, SequenceID id) const {
        auto read = _table.readAccess();
        auto index = read.getEntry(id).getIndex();
        return std::make_pair(v8::Local<v8::Value>::New(isolate, _referenceArray[index]),
                              _referenceArray[index].WrapperClassId());
    }

    ReferenceTableStats dumpStats() const {
        return _table.dumpStats();
    }

    static IndirectV8PersistentTable& get() {
        static auto* kInstance = new IndirectV8PersistentTable();
        return *kInstance;
    }

private:
    ReferenceTable _table;
    std::vector<v8::Persistent<v8::Value, v8::CopyablePersistentTraits<v8::Value>>> _referenceArray;
};

void V8ManagedPointerWrapper::release() {
    _deleter(_opaque);
    IndirectV8PersistentTable::get().release(_sequenceID);
}

IndirectV8Persistent::IndirectV8Persistent(const SequenceID& id) : _id(id) {}

IndirectV8Persistent::IndirectV8Persistent() = default;

IndirectV8Persistent::IndirectV8Persistent(IndirectV8Persistent&& other) noexcept : _id(other._id) {
    other._id = SequenceID();
}

IndirectV8Persistent::IndirectV8Persistent(const IndirectV8Persistent& other) = default;

bool IndirectV8Persistent::isEmpty() const {
    return !IndirectV8PersistentTable::get().contains(_id);
}

size_t IndirectV8Persistent::retainCount() const {
    if (isEmpty()) {
        return 0;
    }

    return IndirectV8PersistentTable::get().getRetainCount(_id);
}

void IndirectV8Persistent::retain() {
    if (!isEmpty()) {
        IndirectV8PersistentTable::get().retain(_id);
    }
}

void IndirectV8Persistent::release() {
    if (!isEmpty()) {
        IndirectV8PersistentTable::get().release(_id);
        _id = SequenceID();
    }
}

std::pair<v8::Local<v8::Value>, uint16_t> IndirectV8Persistent::getWithWrapperID(v8::Isolate* isolate) const {
    if (isEmpty()) {
        return std::make_pair(v8::Local<v8::Value>(), kNoWrapperID);
    }

    return IndirectV8PersistentTable::get().newLocalRef(isolate, _id);
}

v8::Local<v8::Value> IndirectV8Persistent::get(v8::Isolate* isolate) const {
    if (isEmpty()) {
        return v8::Local<v8::Value>();
    }

    return getWithWrapperID(isolate).first;
}

IndirectV8Persistent& IndirectV8Persistent::operator=(const IndirectV8Persistent& other) {
    if (this != &other) {
        _id = other._id;
    }

    return *this;
}

IndirectV8Persistent& IndirectV8Persistent::operator=(IndirectV8Persistent&& other) noexcept {
    if (this != &other) {
        _id = other._id;
        other._id = SequenceID();
    }

    return *this;
}

IndirectV8Persistent IndirectV8Persistent::make(v8::Isolate* isolate, const v8::Local<v8::Value>& value) {
    if (value.IsEmpty()) {
        return IndirectV8Persistent();
    }
    auto id = IndirectV8PersistentTable::get().make(isolate, value, nullptr, nullptr, "v8");
    return IndirectV8Persistent(id);
}

v8::Local<v8::External> IndirectV8Persistent::makeWeakExternal(v8::Isolate* isolate,
                                                               void* value,
                                                               const std::function<void(void*)>& weak_deleter) {
    if (value == nullptr) {
        return v8::External::New(isolate, nullptr);
    }
    auto retval = v8::External::New(isolate, value);

    IndirectV8PersistentTable::get().make(isolate, retval, value, &weak_deleter, "v8-wrapped");
    return retval;
}

ReferenceTableStats IndirectV8Persistent::dumpStats() {
    return IndirectV8PersistentTable::get().dumpStats();
}

} // namespace Valdi::V8
