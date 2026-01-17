//
//  AttributedTextOnTapGestureRecognizer.cpp
//  snap_drawing-macos
//
//  Created by Simon Corsin on 12/22/22.
//

#include "snap_drawing/cpp/Touches/AttributedTextOnTapGestureRecognizer.hpp"
#include "snap_drawing/cpp/Text/AttributedText.hpp"
#include "snap_drawing/cpp/Text/TextLayout.hpp"

namespace snap::drawing {

AttributedTextOnTapGestureRecognizer::AttributedTextOnTapGestureRecognizer(
    const GesturesConfiguration& gesturesConfiguration)
    : TapGestureRecognizer(1, gesturesConfiguration), _touchTolerance(gesturesConfiguration.touchTolerance) {
    setListener(AttributedTextOnTapGestureRecognizer::makeOnTapListener());
}

AttributedTextOnTapGestureRecognizer::~AttributedTextOnTapGestureRecognizer() = default;

void AttributedTextOnTapGestureRecognizer::setTextLayout(const Ref<TextLayout>& textLayout) {
    _textLayout = textLayout;
}

bool AttributedTextOnTapGestureRecognizer::shouldBegin() {
    _onTapAttribute = nullptr;
    if (_textLayout == nullptr) {
        return false;
    }

    auto attachment = _textLayout->getAttachmentAtPoint(getLocation(), _touchTolerance);
    _onTapAttribute = Valdi::castOrNull<AttributedTextOnTapAttribute>(attachment);
    return _onTapAttribute != nullptr;
}

void AttributedTextOnTapGestureRecognizer::forwardTouchEvent(GestureRecognizerState state,
                                                             const Point& location) const {
    if (_onTapAttribute != nullptr) {
        _onTapAttribute->onTap(*this, state, location);
    }
}

void AttributedTextOnTapGestureRecognizer::onReset() {
    _onTapAttribute = nullptr;
    TapGestureRecognizer::onReset();
}

TapGestureRecognizer::Listener AttributedTextOnTapGestureRecognizer::makeOnTapListener() {
    return [](const auto& gesture, auto state, const auto& event) {
        dynamic_cast<const AttributedTextOnTapGestureRecognizer&>(gesture).forwardTouchEvent(state, event);
    };
}

std::string_view AttributedTextOnTapGestureRecognizer::getTypeName() const {
    return "text link";
}

} // namespace snap::drawing
