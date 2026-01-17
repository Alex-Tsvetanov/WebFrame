//
//  SingleTapGestureRecognizer.cpp
//  valdi-skia
//
//  Created by Vincent Brunet on 01/07/21.
//

#include "snap_drawing/cpp/Touches/SingleTapGestureRecognizer.hpp"
#include "snap_drawing/cpp/Touches/TapGestureRecognizer.hpp"

namespace snap::drawing {

SingleTapGestureRecognizer::SingleTapGestureRecognizer(const GesturesConfiguration& gesturesConfiguration)
    : TapGestureRecognizer(1, gesturesConfiguration) {}

SingleTapGestureRecognizer::~SingleTapGestureRecognizer() = default;

std::string_view SingleTapGestureRecognizer::getTypeName() const {
    return "single tap";
}

} // namespace snap::drawing
