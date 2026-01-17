//
//  RefCountableAutoreleasePool.hpp
//  valdi
//
//  Created by Simon Corsin on 2/28/22.
//

#pragma once

#include "utils/base/NonCopyable.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"
#include "valdi_core/cpp/Utils/SmallVector.hpp"

namespace Valdi {

class RefCountableAutoreleasePool : public snap::NonCopyable {
public:
    RefCountableAutoreleasePool();
    ~RefCountableAutoreleasePool();

    void releaseAll();

    static void release(void* ptr);

    // Exposed for tests only
    size_t backingArraySize() const;

    static RefCountableAutoreleasePool* current();
    static RefCountableAutoreleasePool makeNonDeferred();

private:
    RefCountableAutoreleasePool* _parent;
    Valdi::SmallVector<void*, 8> _entries;
    size_t _entriesIndex = 0;

    void append(void* ptr);

    RefCountableAutoreleasePool(bool deferred);
};

} // namespace Valdi
