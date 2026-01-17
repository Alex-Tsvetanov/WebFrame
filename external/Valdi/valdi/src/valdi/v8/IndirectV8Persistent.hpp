//
//  IndirectV8Persistent.hpp
//  valdi-ios
//
//  Created by Ramzy Jaber on 6/13/23.
//

#include <utility>

#include "v8/v8-external.h"
#include "v8/v8-isolate.h"
#include "v8/v8-persistent-handle.h"
#include "valdi_core/cpp/Utils/ReferenceTable.hpp"
#include "valdi_core/cpp/Utils/SequenceIDGenerator.hpp"
#include <utility>
#include <vector>

namespace Valdi::V8 {

constexpr uint16_t kNoWrapperID =
    0; // NOTE(rjaber): This value must be zero, since this is what v8 uses to represent this infomation
constexpr uint16_t kWrappedObjectID = 1;
/*
    NOTE(rjaber):
        There doesn't seem to be a direct way to interact with the garbage collector and let it know that we have
   multiple owners for a Persistent, or Global object. We need this wrapper so we can hide the underlying object and
   maintain a retain/release tracking for the valdi context abstraction. Since we are storing the object in a bridge
   it felt appropriate to delete equality, and normal constructors
*/

class IndirectV8Persistent {
public:
    IndirectV8Persistent();
    IndirectV8Persistent(IndirectV8Persistent&& other) noexcept;
    IndirectV8Persistent(const IndirectV8Persistent& other);
    ~IndirectV8Persistent() = default;
    ;

    size_t retainCount() const;

    bool isEmpty() const;

    std::pair<v8::Local<v8::Value>, uint16_t> getWithWrapperID(v8::Isolate* isolate) const;
    v8::Local<v8::Value> get(v8::Isolate* isolate) const;

    IndirectV8Persistent& operator=(const IndirectV8Persistent& other);
    IndirectV8Persistent& operator=(IndirectV8Persistent&& other) noexcept;

    static IndirectV8Persistent make(v8::Isolate* isolate, const v8::Local<v8::Value>& value);
    static v8::Local<v8::External> makeWeakExternal(v8::Isolate* isolate,
                                                    void* value,
                                                    const std::function<void(void*)>& weak_deleter);

    void retain();
    void release();

    static ReferenceTableStats dumpStats();

private:
    SequenceID _id;

    explicit IndirectV8Persistent(const SequenceID& id);
};
} // namespace Valdi::V8