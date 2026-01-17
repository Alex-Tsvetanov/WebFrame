//
//  InterpolationFunction.hpp
//  valdi-skia
//
//  Created by Simon Corsin on 7/3/20.
//

#pragma once

#include "valdi_core/cpp/Utils/Function.hpp"

namespace snap::drawing {

using InterpolationFunction = Valdi::Function<double(double)>;

class InterpolationFunctions {
public:
    static InterpolationFunction linear();
    static InterpolationFunction systemDefault();
    static InterpolationFunction easeInOut();
    static InterpolationFunction easeIn();
    static InterpolationFunction easeOut();
    static InterpolationFunction strongEaseOut();
    static InterpolationFunction viscousFluid();
};

} // namespace snap::drawing
