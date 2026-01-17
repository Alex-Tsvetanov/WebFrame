//
//  Aliases.hpp
//  valdi-desktop-apple
//
//  Created by Simon Corsin on 7/2/20.
//

#pragma once

#include "valdi_core/cpp/Utils/Shared.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"

namespace snap::drawing {

template<typename T>
using Ref = Valdi::Ref<T>;

using String = Valdi::StringBox;

} // namespace snap::drawing
