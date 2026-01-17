//
//  StandaloneView.cpp
//  valdi-standalone-pc
//
//  Created by Simon Corsin on 10/14/19.
//

#include "valdi/standalone_runtime/StandaloneView.hpp"
#include "valdi/runtime/Context/ViewNode.hpp"

#include "utils/debugging/Assert.hpp"
#include "valdi_core/cpp/Resources/LoadedAsset.hpp"
#include "valdi_core/cpp/Utils/Format.hpp"

namespace Valdi {

StandaloneView::StandaloneView(const StringBox& className) : StandaloneView(className, false, false) {}

StandaloneView::StandaloneView(const StringBox& className, bool keepAttributesHistory, bool allowPooling)
    : _className(className), _attributes(makeShared<ValueMap>()), _allowPooling(allowPooling) {
    if (keepAttributesHistory) {
        _attributesHistory = Valdi::makeShared<AttributesHistory>();
    }
}

StandaloneView::~StandaloneView() = default;

Ref<StandaloneView> StandaloneView::getChild(size_t index) const {
    SC_ASSERT(index < _children.size());

    return _children[index];
}

void StandaloneView::removeFromParent() {
    auto parent = getParent();
    if (parent != nullptr) {
        _parent.reset();

        auto it = parent->_children.begin();
        while (it != parent->_children.end()) {
            if (it->get() == this) {
                parent->_children.erase(it);
                return;
            }
            ++it;
        }

        SC_ASSERT_FAIL("Could not resolve self in parent");
    }
}

void StandaloneView::addChild(const Ref<StandaloneView>& child) {
    addChild(child, _children.size());
}

void StandaloneView::addChild(const Ref<StandaloneView>& child, size_t index) {
    child->removeFromParent();

    SC_ASSERT(index <= _children.size());

    _children.insert(_children.begin() + index, child);
    child->_parent = weakRef(this);
}

const std::vector<Ref<StandaloneView>>& StandaloneView::getChildren() const {
    return _children;
}

void StandaloneView::setAttribute(const StringBox& name, const Value& value) {
    if (value.isNullOrUndefined()) {
        const auto& it = _attributes->find(name);
        if (it != _attributes->end()) {
            _attributes->erase(it);
        }
    } else {
        (*_attributes)[name] = value;
        if (_attributesHistory != nullptr) {
            (*_attributesHistory)[name].push_back(value);
        }
    }
}

const Value& StandaloneView::getAttribute(const StringBox& name) const {
    const auto& it = _attributes->find(name);
    if (it == _attributes->end()) {
        return Value::undefinedRef();
    }

    return it->second;
}

Value StandaloneView::serialize() const {
    static auto kClassName = STRING_LITERAL("className");
    static auto kChildren = STRING_LITERAL("children");
    static auto kAttributes = STRING_LITERAL("attributes");

    auto map = makeShared<ValueMap>();
    (*map)[kClassName] = Valdi::Value(_className);

    auto children = ValueArray::make(this->_children.size());

    size_t i = 0;
    for (const auto& child : this->_children) {
        (*children)[i++] = child->serialize();
    }

    (*map)[kChildren] = Valdi::Value(children);

    (*map)[kAttributes] = Valdi::Value(_attributes);

    return Valdi::Value(map);
}

std::string StandaloneView::toXML() const {
    std::ostringstream ss;

    outputXML(ss, 0);

    return ss.str();
}

void StandaloneView::outputXML(std::ostringstream& stream, int indent) const {
    for (int i = 0; i < indent; i++) {
        stream << "  ";
    }

    stream << "<" << _className.toStringView();

    for (const auto& it : *_attributes) {
        stream << " " << it.first << "=\"" << it.second.toString() << "\"";
    }

    if (_children.empty()) {
        stream << "/>" << std::endl;
    } else {
        stream << ">" << std::endl;

        for (const auto& child : _children) {
            child->outputXML(stream, indent + 1);
        }

        for (int i = 0; i < indent; i++) {
            stream << "  ";
        }

        stream << "</" << _className.toStringView() << ">" << std::endl;
    }
}

bool StandaloneView::operator!=(const StandaloneView& other) const {
    return !(*this == other);
}

bool StandaloneView::operator==(const StandaloneView& other) const {
    if (_className != other._className) {
        return false;
    }

    if (_children.size() != other._children.size()) {
        return false;
    }

    if (*_attributes != *other._attributes) {
        return false;
    }

    for (size_t i = 0; i < _children.size(); i++) {
        if (*getChild(i) != *other.getChild(i)) {
            return false;
        }
    }

    return true;
}

Ref<StandaloneView> StandaloneView::unwrap(const Ref<View>& view) {
    return castOrNull<StandaloneView>(view);
}

StringBox StandaloneView::getViewClassName() const {
    return _className;
}

const Shared<AttributesHistory>& StandaloneView::getAttributesHistory() const {
    return _attributesHistory;
}

void StandaloneView::setFrame(const Frame& frame) {
    _frame = frame;
}

const Frame& StandaloneView::getFrame() const {
    return _frame;
}

Ref<StandaloneView> StandaloneView::getParent() const {
    return _parent.lock();
}

void StandaloneView::invalidateLayout() {
    _invalidateLayoutCount++;
    setLayoutInvalidated(true);
}

int StandaloneView::getInvalidateLayoutCount() const {
    return _invalidateLayoutCount;
}

int StandaloneView::getLayoutDidBecomeDirtyCount() const {
    return _layoutDidBecomeDirtyCount;
}

bool StandaloneView::isLayoutInvalidated() const {
    return _layoutInvalidated;
}

void StandaloneView::setLayoutInvalidated(bool layoutInvalidated) {
    _layoutInvalidated = layoutInvalidated;
}

snap::valdi_core::Platform StandaloneView::getPlatform() const {
    return snap::valdi_core::Platform::Ios;
}

void StandaloneView::incrementLayoutDidBecomeDirty() {
    _layoutDidBecomeDirtyCount++;
}

bool StandaloneView::isRightToLeft() const {
    return _isRightToLeft;
}

void StandaloneView::setIsRightToLeft(bool isRightToLeft) {
    _isRightToLeft = isRightToLeft;
}

bool StandaloneView::allowsPooling() const {
    return _allowPooling;
}

const Valdi::Ref<Valdi::LoadedAsset>& StandaloneView::getLoadedAsset() const {
    return _loadedAsset;
}

void StandaloneView::setLoadedAsset(const Valdi::Ref<Valdi::LoadedAsset>& loadedAsset) {
    _loadedAsset = loadedAsset;
}

VALDI_CLASS_IMPL(StandaloneView)

} // namespace Valdi
