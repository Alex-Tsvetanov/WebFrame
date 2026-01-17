
#pragma once

#include "valdi/runtime/Context/ViewNode.hpp"
#include "valdi_core/cpp/Utils/Function.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"

namespace Valdi {

using AttributePostprocessor = Function<Result<Value>(ViewNode& viewNode, const Value& value)>;

}
