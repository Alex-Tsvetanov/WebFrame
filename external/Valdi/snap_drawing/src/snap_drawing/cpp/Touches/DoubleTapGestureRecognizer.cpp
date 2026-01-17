//
//  DoubleTapGestureRecognizer.cpp
//  valdi-skia
//
//  Created by Vincent Brunet on 01/07/21.
//

#include "snap_drawing/cpp/Touches/DoubleTapGestureRecognizer.hpp"
#include "snap_drawing/cpp/Touches/TapGestureRecognizer.hpp"

namespace snap::drawing {

DoubleTapGestureRecognizer::DoubleTapGestureRecognizer(const GesturesConfiguration& gesturesConfiguration)
    : TapGestureRecognizer(2, gesturesConfiguration) {}

DoubleTapGestureRecognizer::~DoubleTapGestureRecognizer() = default;

std::string_view DoubleTapGestureRecognizer::getTypeName() const {
    return "double tap";
}

} // namespace snap::drawing
