//
//  AttributePreprocessor.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 8/2/21.
//

#pragma once

#include "valdi_core/cpp/Utils/Function.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"

namespace Valdi {

using AttributePreprocessor = Function<Result<Value>(const Value&)>;

}
