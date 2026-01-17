//
//  ContextEntry.cpp
//  valdi
//
//  Created by Simon Corsin on 7/7/23.
//

#include "valdi/runtime/Context/ContextEntry.hpp"

namespace Valdi {

ContextEntry::ContextEntry(const Ref<Context>& context) : _oldContext(Context::current()) {
    Context::setCurrent(context);
}

ContextEntry::~ContextEntry() {
    Context::setCurrent(std::move(_oldContext));
}

} // namespace Valdi
