//
//  ContextEntry.hpp
//  valdi
//
//  Created by Simon Corsin on 7/7/23.
//

#pragma once

#include "utils/base/NonCopyable.hpp"
#include "valdi/runtime/Context/Context.hpp"

namespace Valdi {

class ContextEntry : public snap::NonCopyable {
public:
    explicit ContextEntry(const Ref<Context>& context);
    ~ContextEntry();

private:
    Ref<Context> _oldContext;
};

} // namespace Valdi
