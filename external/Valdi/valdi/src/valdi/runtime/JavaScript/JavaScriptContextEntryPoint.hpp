//
//  JavaScriptContextEntryPoint.hpp
//  valdi
//
//  Created by Simon Corsin on 5/10/21.
//

#pragma once

#include "valdi/runtime/Context/ContextEntry.hpp"
#include "valdi/runtime/Utils/RefCountableAutoreleasePool.hpp"

namespace Valdi {

class JavaScriptContextEntry : public ContextEntry {
public:
    explicit JavaScriptContextEntry(const Ref<Context>& context);
    ~JavaScriptContextEntry();

private:
    RefCountableAutoreleasePool _autoreleasePool;
};

} // namespace Valdi
