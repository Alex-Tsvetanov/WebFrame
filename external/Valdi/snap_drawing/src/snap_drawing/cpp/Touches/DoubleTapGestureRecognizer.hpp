//
//  DoubleTapGestureRecognizer.hpp
//  valdi-skia
//
//  Created by Vincent Brunet on 01/07/21.
//

#pragma once

#include "snap_drawing/cpp/Touches/TapGestureRecognizer.hpp"

namespace snap::drawing {

class DoubleTapGestureRecognizer : public TapGestureRecognizer {
public:
    DoubleTapGestureRecognizer(const GesturesConfiguration& gesturesConfiguration);
    ~DoubleTapGestureRecognizer() override;

protected:
    std::string_view getTypeName() const final;
};

} // namespace snap::drawing
