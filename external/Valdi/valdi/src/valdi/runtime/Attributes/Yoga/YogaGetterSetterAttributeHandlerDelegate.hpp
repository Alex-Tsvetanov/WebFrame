//
//  YogaGetterSetterAttributeHandlerDelegate.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 8/3/21.
//

#pragma once

#include "valdi/runtime/Attributes/Yoga/YogaAttributeHandlerDelegate.hpp"

namespace Valdi {

template<typename T>
struct YGNodeValueGetterSetter {
    using Setter = void (*)(YGNodeRef, T);
    using Getter = T (*)(YGNodeRef);

    Getter get;
    Setter set;

    constexpr YGNodeValueGetterSetter(Getter getter, Setter setter) : get(getter), set(setter) {}
};

template<typename T>
class YogaGetterSetterAttributeHandlerDelegate : public YogaAttributeHandlerDelegate {
public:
    explicit YogaGetterSetterAttributeHandlerDelegate(YGNodeValueGetterSetter<T> getterSetter)
        : _getterSetter(getterSetter) {}

protected:
    void onReset(YGNodeRef node, YGNodeRef defaultYogaNode) override {
        auto defaultValue = _getterSetter.get(defaultYogaNode);
        _getterSetter.set(node, defaultValue);
    }

    inline Result<Void> setValue(YGNodeRef node, const T& value) {
        _getterSetter.set(node, value);
        return Void();
    }

private:
    YGNodeValueGetterSetter<T> _getterSetter;
};

} // namespace Valdi
