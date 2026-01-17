//
//  AttributesBinderHelper.cpp
//  valdi
//
//  Created by Simon Corsin on 8/6/21.
//

#include "valdi/runtime/Attributes/AttributesBinderHelper.hpp"
#include "valdi/runtime/Attributes/AttributeHandlerDelegate.hpp"
#include "valdi/runtime/Attributes/AttributeHandlerDelegateWithCallable.hpp"
#include "valdi/runtime/Attributes/CompositeAttributeUtils.hpp"

namespace Valdi {

template<typename T>
struct ViewNodeSetterHolder {
    ViewNodeSetter<T> setter = nullptr;
    ViewNodeSetterWithViewTransactionScope<T> setterWithViewTransactionScope = nullptr;

    explicit ViewNodeSetterHolder(ViewNodeSetter<T> setter) : setter(setter) {}
    explicit ViewNodeSetterHolder(ViewNodeSetterWithViewTransactionScope<T> setterWithViewTransactionScope)
        : setterWithViewTransactionScope(setterWithViewTransactionScope) {}

    void call(ViewTransactionScope& viewTransactionScope, ViewNode& viewNode, T&& value) {
        if (setter != nullptr) {
            (viewNode.*setter)(std::move(value));
        } else if (setterWithViewTransactionScope != nullptr) {
            (viewNode.*setterWithViewTransactionScope)(viewTransactionScope, std::move(value));
        }
    }
};

template<typename T>
class ViewNodeAttributeHandlerDelegateBase : public AttributeHandlerDelegate {
public:
    explicit ViewNodeAttributeHandlerDelegateBase(ViewNodeSetterHolder<T> setter) : _setter(setter) {}

protected:
    ViewNodeSetterHolder<T> _setter;
};

class ViewNodeCallbackAttributeHandlerDelegate : public ViewNodeAttributeHandlerDelegateBase<Ref<ValueFunction>> {
public:
    explicit ViewNodeCallbackAttributeHandlerDelegate(ViewNodeSetterHolder<Ref<ValueFunction>> setter)
        : ViewNodeAttributeHandlerDelegateBase(setter) {}

    Result<Void> onApply(ViewTransactionScope& viewTransactionScope,
                         ViewNode& viewNode,
                         const Ref<View>& /*view*/,
                         const StringBox& /*name*/,
                         const Value& value,
                         const Ref<Animator>& /*animator*/) override {
        auto functionRef = value.getFunctionRef();
        if (functionRef == nullptr && !value.isNullOrUndefined()) {
            return Error("Not a function");
        }
        _setter.call(viewTransactionScope, viewNode, std::move(functionRef));
        return Void();
    }

    void onReset(ViewTransactionScope& viewTransactionScope,
                 ViewNode& viewNode,
                 const Ref<View>& /*view*/,
                 const StringBox& /*name*/,
                 const Ref<Animator>& /*animator*/) override {
        _setter.call(viewTransactionScope, viewNode, nullptr);
    }
};

class ViewNodeBooleanAttributeHandlerDelegate : public ViewNodeAttributeHandlerDelegateBase<bool> {
public:
    explicit ViewNodeBooleanAttributeHandlerDelegate(ViewNodeSetterHolder<bool> setter)
        : ViewNodeAttributeHandlerDelegateBase(setter) {}

    Result<Void> onApply(ViewTransactionScope& viewTransactionScope,
                         ViewNode& viewNode,
                         const Ref<View>& /*view*/,
                         const StringBox& /*name*/,
                         const Value& value,
                         const Ref<Animator>& /*animator*/) override {
        _setter.call(viewTransactionScope, viewNode, value.toBool());
        return Void();
    }

    void onReset(ViewTransactionScope& viewTransactionScope,
                 ViewNode& viewNode,
                 const Ref<View>& /*view*/,
                 const StringBox& /*name*/,
                 const Ref<Animator>& /*animator*/) override {
        _setter.call(viewTransactionScope, viewNode, false);
    }
};

class ViewNodeStringAttributeHandlerDelegate : public ViewNodeAttributeHandlerDelegateBase<const StringBox&> {
public:
    explicit ViewNodeStringAttributeHandlerDelegate(ViewNodeSetterHolder<const StringBox&> setter)
        : ViewNodeAttributeHandlerDelegateBase(setter) {}

    Result<Void> onApply(ViewTransactionScope& viewTransactionScope,
                         ViewNode& viewNode,
                         const Ref<View>& /*view*/,
                         const StringBox& /*name*/,
                         const Value& value,
                         const Ref<Animator>& /*animator*/) override {
        _setter.call(viewTransactionScope, viewNode, value.toStringBox());
        return Void();
    }

    void onReset(ViewTransactionScope& viewTransactionScope,
                 ViewNode& viewNode,
                 const Ref<View>& /*view*/,
                 const StringBox& /*name*/,
                 const Ref<Animator>& /*animator*/) override {
        _setter.call(viewTransactionScope, viewNode, StringBox());
    }
};

class ViewNodeFloatAttributeHandlerDelegate : public ViewNodeAttributeHandlerDelegateBase<float> {
public:
    explicit ViewNodeFloatAttributeHandlerDelegate(ViewNodeSetterHolder<float> setter)
        : ViewNodeAttributeHandlerDelegateBase(setter) {}

    Result<Void> onApply(ViewTransactionScope& viewTransactionScope,
                         ViewNode& viewNode,
                         const Ref<View>& /*view*/,
                         const StringBox& /*name*/,
                         const Value& value,
                         const Ref<Animator>& /*animator*/) override {
        _setter.call(viewTransactionScope, viewNode, value.toFloat());
        return Void();
    }

    void onReset(ViewTransactionScope& viewTransactionScope,
                 ViewNode& viewNode,
                 const Ref<View>& /*view*/,
                 const StringBox& /*name*/,
                 const Ref<Animator>& /*animator*/) override {
        _setter.call(viewTransactionScope, viewNode, 0);
    }
};

class ViewNodeIntAttributeHandlerDelegate : public ViewNodeAttributeHandlerDelegateBase<int> {
public:
    explicit ViewNodeIntAttributeHandlerDelegate(ViewNodeSetterHolder<int> setter)
        : ViewNodeAttributeHandlerDelegateBase(setter) {}

    Result<Void> onApply(ViewTransactionScope& viewTransactionScope,
                         ViewNode& viewNode,
                         const Ref<View>& /*view*/,
                         const StringBox& /*name*/,
                         const Value& value,
                         const Ref<Animator>& /*animator*/) override {
        _setter.call(viewTransactionScope, viewNode, value.toInt());

        return Void();
    }

    void onReset(ViewTransactionScope& viewTransactionScope,
                 ViewNode& viewNode,
                 const Ref<View>& /*view*/,
                 const StringBox& /*name*/,
                 const Ref<Animator>& /*animator*/) override {
        _setter.call(viewTransactionScope, viewNode, 0);
    }
};

class ViewNodeEnumAttributeHandlerDelegate : public AttributeHandlerDelegate {
public:
    explicit ViewNodeEnumAttributeHandlerDelegate(ViewNodeSetter<int> setter,
                                                  const Shared<FlatMap<StringBox, int>>& map,
                                                  int defaultValue)
        : _setter(setter), _map(map), _defaultValue(defaultValue) {}

    Result<Void> onApply(ViewTransactionScope& /*viewTransactionScope*/,
                         ViewNode& viewNode,
                         const Ref<View>& /*view*/,
                         const StringBox& /*name*/,
                         const Value& value,
                         const Ref<Animator>& /*animator*/) override {
        auto enumValueStr = value.toStringBox();
        const auto& it = _map->find(enumValueStr);
        if (it == _map->end()) {
            auto acceptableValues = ValueArray::make(_map->size());
            size_t i = 0;
            for (const auto& valueIt : *_map) {
                (*acceptableValues)[i++] = Value(valueIt.first);
            }
            return Error(STRING_FORMAT(
                "Invalid value '{}', acceptable values are '{}'", enumValueStr, Value(acceptableValues).toString()));
        }
        (viewNode.*_setter)(it->second);
        return Void();
    }

    void onReset(ViewTransactionScope& /*viewTransactionScope*/,
                 ViewNode& viewNode,
                 const Ref<View>& /*view*/,
                 const StringBox& /*name*/,
                 const Ref<Animator>& /*animator*/) override {
        (viewNode.*_setter)(_defaultValue);
    }

private:
    ViewNodeSetter<int> _setter;
    Shared<FlatMap<StringBox, int>> _map;
    int _defaultValue;
};

AttributesBinderHelper::AttributesBinderHelper(AttributeIds& attributeIds, AttributeHandlerById& attributes)
    : _attributeIds(attributeIds), _attributes(attributes) {}

void AttributesBinderHelper::bindViewNodeCallback(std::string_view attribute,
                                                  ViewNodeSetter<Ref<ValueFunction>> setter) {
    bind(attribute, makeShared<ViewNodeCallbackAttributeHandlerDelegate>(ViewNodeSetterHolder(setter)));
}

void AttributesBinderHelper::bindViewNodeCallback(std::string_view attribute,
                                                  ViewNodeSetterWithViewTransactionScope<Ref<ValueFunction>> setter) {
    bind(attribute, makeShared<ViewNodeCallbackAttributeHandlerDelegate>(ViewNodeSetterHolder(setter)));
}

void AttributesBinderHelper::bindViewNodeBoolean(std::string_view attribute, ViewNodeSetter<bool> setter) {
    bind(attribute, makeShared<ViewNodeBooleanAttributeHandlerDelegate>(ViewNodeSetterHolder(setter)));
}

void AttributesBinderHelper::bindViewNodeBoolean(std::string_view attribute,
                                                 ViewNodeSetterWithViewTransactionScope<bool> setter) {
    bind(attribute, makeShared<ViewNodeBooleanAttributeHandlerDelegate>(ViewNodeSetterHolder(setter)));
}

void AttributesBinderHelper::bindViewNodeString(std::string_view attribute, ViewNodeSetter<const StringBox&> setter) {
    bind(attribute, makeShared<ViewNodeStringAttributeHandlerDelegate>(ViewNodeSetterHolder(setter)));
}

void AttributesBinderHelper::bindViewNodeString(std::string_view attribute,
                                                ViewNodeSetterWithViewTransactionScope<const StringBox&> setter) {
    bind(attribute, makeShared<ViewNodeStringAttributeHandlerDelegate>(ViewNodeSetterHolder(setter)));
}

void AttributesBinderHelper::bindViewNodeFloat(std::string_view attribute, ViewNodeSetter<float> setter) {
    bind(attribute, makeShared<ViewNodeFloatAttributeHandlerDelegate>(ViewNodeSetterHolder(setter)));
}

void AttributesBinderHelper::bindViewNodeFloat(std::string_view attribute,
                                               ViewNodeSetterWithViewTransactionScope<float> setter) {
    bind(attribute, makeShared<ViewNodeFloatAttributeHandlerDelegate>(ViewNodeSetterHolder(setter)));
}

void AttributesBinderHelper::bindViewNodeInt(std::string_view attribute, ViewNodeSetter<int> setter) {
    bind(attribute, makeShared<ViewNodeIntAttributeHandlerDelegate>(ViewNodeSetterHolder(setter)));
}

void AttributesBinderHelper::bindViewNodeInt(std::string_view attribute,
                                             ViewNodeSetterWithViewTransactionScope<int> setter) {
    bind(attribute, makeShared<ViewNodeIntAttributeHandlerDelegate>(ViewNodeSetterHolder(setter)));
}

void AttributesBinderHelper::bindViewNodeEnum(std::string_view attribute,
                                              ViewNodeSetter<int> setter,
                                              const Shared<FlatMap<StringBox, int>>& map,
                                              int defaultValue) {
    bind(attribute, makeShared<ViewNodeEnumAttributeHandlerDelegate>(setter, map, defaultValue));
}

void AttributesBinderHelper::bind(std::string_view attribute,
                                  AttributeHandlerDelegateWithCallable::Applier applier,
                                  AttributeHandlerDelegateWithCallable::Resetter resetter) {
    bind(attribute, makeShared<AttributeHandlerDelegateWithCallable>(applier, resetter));
}

void AttributesBinderHelper::bind(std::string_view attribute, const Ref<AttributeHandlerDelegate>& delegate) {
    auto internedAttributeName = StringCache::getGlobal().makeString(attribute);
    auto attributeId = _attributeIds.getIdForName(internedAttributeName);
    _attributes[attributeId] = AttributeHandler(attributeId, internedAttributeName, delegate, nullptr, false, false);
}

void AttributesBinderHelper::bindComposite(std::string_view attribute,
                                           std::vector<CompositeAttributePart>&& compositeParts,
                                           AttributeHandlerDelegateWithCallable::Applier applier,
                                           AttributeHandlerDelegateWithCallable::Resetter resetter) {
    auto compositeName = StringCache::getGlobal().makeString(attribute);
    auto compositeAttribute = Valdi::makeShared<CompositeAttribute>(
        _attributeIds.getIdForName(compositeName), compositeName, std::move(compositeParts));
    auto delegate = makeShared<AttributeHandlerDelegateWithCallable>(applier, resetter);

    Valdi::bindCompositeAttribute(_attributeIds, _attributes, compositeAttribute, delegate, false);
}

} // namespace Valdi
