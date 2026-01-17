//
//  JavaScriptContextEntryPoint.cpp
//  valdi
//
//  Created by Simon Corsin on 5/10/21.
//

#include "valdi/runtime/JavaScript/JavaScriptContextEntryPoint.hpp"

namespace Valdi {

JavaScriptContextEntry::JavaScriptContextEntry(const Ref<Context>& context) : ContextEntry(context) {}

JavaScriptContextEntry::~JavaScriptContextEntry() {
    _autoreleasePool.releaseAll();
}

} // namespace Valdi
