//
//  AttributedTextOnTapGestureRecognizer.hpp
//  snap_drawing-macos
//
//  Created by Simon Corsin on 12/22/22.
//

#pragma once

#include "snap_drawing/cpp/Touches/TapGestureRecognizer.hpp"

namespace snap::drawing {

class TextLayout;
class AttributedTextOnTapAttribute;

class AttributedTextOnTapGestureRecognizer : public TapGestureRecognizer {
public:
    AttributedTextOnTapGestureRecognizer(const GesturesConfiguration& gesturesConfiguration);
    ~AttributedTextOnTapGestureRecognizer() override;

    void setTextLayout(const Ref<TextLayout>& textLayout);

protected:
    bool shouldBegin() override;
    void onReset() override;

    std::string_view getTypeName() const final;

private:
    Ref<TextLayout> _textLayout;
    Ref<AttributedTextOnTapAttribute> _onTapAttribute;
    Scalar _touchTolerance;

    void forwardTouchEvent(GestureRecognizerState state, const Point& location) const;

    static TapGestureRecognizer::Listener makeOnTapListener();
};

} // namespace snap::drawing
