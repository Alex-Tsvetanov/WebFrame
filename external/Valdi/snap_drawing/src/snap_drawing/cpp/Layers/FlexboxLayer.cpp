//
//  FlexboxLayer.cpp
//  snap_drawing-macos
//
//  Created by Simon Corsin on 3/10/23.
//

#include "snap_drawing/cpp/Layers/FlexboxLayer.hpp"
#include <yoga/YGNode.h>

namespace snap::drawing {

static YGConfigRef makeYogaConfig() {
    auto* config = YGConfigNew();
    YGConfigSetExperimentalFeatureEnabled(config, YGExperimentalFeatureWebFlexBasis, true);
    return config;
}

static YGConfigRef getYogaConfig() {
    static auto* kYogaConfig = makeYogaConfig();

    return kYogaConfig;
}

struct FlexboxNode : public Valdi::SimpleRefCountable {
    YGNodeRef yogaNode;

    FlexboxNode() : yogaNode(YGNodeNewWithConfig(getYogaConfig())) {}

    ~FlexboxNode() override {
        YGNodeFree(yogaNode);
    }

    void calculateLayout(Size size, bool isRightToLeft) const {
        auto ownerWidth =
            (size.width == std::numeric_limits<Scalar>::max() || std::isnan(size.width)) ? YGUndefined : size.width;
        auto ownerHeight =
            (size.height == std::numeric_limits<Scalar>::max() || std::isnan(size.height)) ? YGUndefined : size.height;

        YGNodeCalculateLayout(yogaNode, ownerWidth, ownerHeight, isRightToLeft ? YGDirectionRTL : YGDirectionLTR);
    }

    void setLayoutDirty() const {
        if (yogaNode->hasMeasureFunc()) {
            YGNodeMarkDirty(yogaNode);
        }
    }

    Rect getFrame() const {
        return Rect::makeXYWH(sanitizeYogaValue(YGNodeLayoutGetLeft(yogaNode)),
                              sanitizeYogaValue(YGNodeLayoutGetTop(yogaNode)),
                              sanitizeYogaValue(YGNodeLayoutGetWidth(yogaNode)),
                              sanitizeYogaValue(YGNodeLayoutGetHeight(yogaNode)));
    }

private:
    static inline float sanitizeYogaValue(float yogaValue) {
        if (std::isnan(yogaValue)) {
            return 0.0;
        }

        return yogaValue;
    }
};

static YGFloatOptional toOptional(const std::optional<Scalar>& value) {
    if (value) {
        return YGFloatOptional(value.value());
    } else {
        return YGFloatOptional();
    }
}

FlexboxAttributes::FlexboxAttributes(YGStyle* style) : _style(style) {}

FlexboxAttributes& FlexboxAttributes::setDirection(YGDirection value) {
    _style->direction() = value;
    return *this;
}

FlexboxAttributes& FlexboxAttributes::setFlexDirection(YGFlexDirection value) {
    _style->flexDirection() = value;
    return *this;
}

FlexboxAttributes& FlexboxAttributes::setJustifyContent(YGJustify value) {
    _style->justifyContent() = value;
    return *this;
}

FlexboxAttributes& FlexboxAttributes::setAlignItems(YGAlign value) {
    _style->alignItems() = value;
    return *this;
}

FlexboxAttributes& FlexboxAttributes::setAlignContent(YGAlign value) {
    _style->alignContent() = value;
    return *this;
}

FlexboxAttributes& FlexboxAttributes::setAlignSelf(YGAlign value) {
    _style->alignSelf() = value;
    return *this;
}

FlexboxAttributes& FlexboxAttributes::setPadding(YGEdge edge, FlexValue value) {
    _style->padding()[edge] = value.value;
    return *this;
}

FlexboxAttributes& FlexboxAttributes::setMargin(YGEdge edge, FlexValue value) {
    _style->margin()[edge] = value.value;
    return *this;
}

FlexboxAttributes& FlexboxAttributes::setBorder(YGEdge edge, FlexValue value) {
    _style->border()[edge] = value.value;
    return *this;
}

FlexboxAttributes& FlexboxAttributes::setPosition(YGEdge edge, FlexValue value) {
    _style->position()[edge] = value.value;
    return *this;
}

FlexboxAttributes& FlexboxAttributes::setPositionType(YGPositionType value) {
    _style->positionType() = value;
    return *this;
}

FlexboxAttributes& FlexboxAttributes::setFlexWrap(YGWrap value) {
    _style->flexWrap() = value;
    return *this;
}

FlexboxAttributes& FlexboxAttributes::setOverflow(YGOverflow value) {
    _style->overflow() = value;
    return *this;
}

FlexboxAttributes& FlexboxAttributes::setDisplay(YGDisplay value) {
    _style->display() = value;
    return *this;
}

FlexboxAttributes& FlexboxAttributes::setFlex(std::optional<Scalar> value) {
    _style->flex() = toOptional(value);
    return *this;
}

FlexboxAttributes& FlexboxAttributes::setFlexGrow(std::optional<Scalar> value) {
    _style->flexGrow() = toOptional(value);
    return *this;
}

FlexboxAttributes& FlexboxAttributes::setFlexShrink(std::optional<Scalar> value) {
    _style->flexShrink() = toOptional(value);
    return *this;
}

FlexboxAttributes& FlexboxAttributes::setFlexBasis(FlexValue value) {
    _style->flexBasis() = value.value;
    return *this;
}

FlexboxAttributes& FlexboxAttributes::setWidth(FlexValue value) {
    _style->dimensions()[YGDimensionWidth] = value.value;
    return *this;
}

FlexboxAttributes& FlexboxAttributes::setHeight(FlexValue value) {
    _style->dimensions()[YGDimensionHeight] = value.value;
    return *this;
}

FlexboxAttributes& FlexboxAttributes::setMinWidth(FlexValue value) {
    _style->minDimensions()[YGDimensionWidth] = value.value;
    return *this;
}

FlexboxAttributes& FlexboxAttributes::setMinHeight(FlexValue value) {
    _style->minDimensions()[YGDimensionHeight] = value.value;
    return *this;
}

FlexboxAttributes& FlexboxAttributes::setMaxWidth(FlexValue value) {
    _style->maxDimensions()[YGDimensionWidth] = value.value;
    return *this;
}

FlexboxAttributes& FlexboxAttributes::setMaxHeight(FlexValue value) {
    _style->maxDimensions()[YGDimensionHeight] = value.value;
    return *this;
}

FlexboxAttributes& FlexboxAttributes::setAspectRatio(std::optional<Scalar> value) {
    _style->aspectRatio() = toOptional(value);
    return *this;
}

void onYogaNodeDirty(YGNodeRef node) {
    auto* layer = reinterpret_cast<snap::drawing::Layer*>(node->getContext());
    if (layer == nullptr) {
        return;
    }

    layer->setNeedsLayout();
}

YGSize onYogaMeasure(YGNodeRef node, float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode) {
    auto* layer = reinterpret_cast<snap::drawing::Layer*>(node->getContext());
    if (layer == nullptr) {
        return {.width = 0, .height = 0};
    }

    auto outputSize = layer->sizeThatFits(
        Size::make(widthMode == YGMeasureModeUndefined ? std::numeric_limits<Scalar>::max() : width,
                   heightMode == YGMeasureModeUndefined ? std::numeric_limits<Scalar>::max() : height));

    return {.width = outputSize.width, .height = outputSize.height};
}

static Ref<FlexboxNode> createAndAssociateFlexboxNode(Layer* layer, bool isOwner) {
    auto flexboxNode = Valdi::makeShared<FlexboxNode>();
    layer->setAttachedData(flexboxNode);

    flexboxNode->yogaNode->setContext(static_cast<snap::drawing::Layer*>(layer));
    flexboxNode->yogaNode->setDirtiedFunc(&onYogaNodeDirty);

    if (!isOwner) {
        flexboxNode->yogaNode->setMeasureFunc(&onYogaMeasure);
    }

    return flexboxNode;
}

static Ref<FlexboxNode> getFlexboxNode(const Layer* layer) {
    return Valdi::castOrNull<FlexboxNode>(layer->getAttachedData());
}

static Ref<FlexboxNode> mustGetFlexboxNode(const Layer* layer) {
    auto node = getFlexboxNode(layer);
    SC_ASSERT_NOTNULL(node);
    return node;
}

static Ref<FlexboxNode> getOrCreateFlexboxNode(Layer* layer) {
    auto flexboxNode = getFlexboxNode(layer);
    if (flexboxNode == nullptr) {
        flexboxNode = createAndAssociateFlexboxNode(layer, false);
    }

    return flexboxNode;
}

FlexboxLayer::FlexboxLayer(const Ref<Resources>& resources) : Layer(resources) {}
FlexboxLayer::~FlexboxLayer() = default;

void FlexboxLayer::onInitialize() {
    createAndAssociateFlexboxNode(this, true);
}

Size FlexboxLayer::sizeThatFits(Size maxSize) {
    auto node = getOrCreateFlexboxNode(this);

    node->calculateLayout(maxSize, isRightToLeft());
    return node->getFrame().size();
}

void FlexboxLayer::onBoundsChanged() {
    Layer::onBoundsChanged();

    setNeedsLayout();
}

void FlexboxLayer::onLayout() {
    Layer::onLayout();

    auto node = mustGetFlexboxNode(this);
    if (YGNodeGetOwner(node->yogaNode) == nullptr) {
        // we are the root node, calculate the layout starting from us
        node->calculateLayout(getFrame().size(), isRightToLeft());
    }

    auto childrenSize = getChildrenSize();
    for (size_t i = 0; i < childrenSize; i++) {
        auto child = getChild(i);
        auto childNode = mustGetFlexboxNode(child.get());

        child->setFrame(childNode->getFrame());
    }
}

FlexboxAttributes FlexboxLayer::updateLayoutAttributes() {
    return updateLayoutAttributesForLayer(this);
}

FlexboxAttributes FlexboxLayer::updateLayoutAttributesForLayer(const Ref<Layer>& layer) {
    return updateLayoutAttributesForLayer(layer.get());
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
FlexboxAttributes FlexboxLayer::updateLayoutAttributesForLayer(Layer* layer) {
    auto flexboxNode = mustGetFlexboxNode(layer);
    auto& style = flexboxNode->yogaNode->getStyle();

    flexboxNode->setLayoutDirty();

    return FlexboxAttributes(&style);
}

void FlexboxLayer::requestLayout(ILayer* layer) {
    Layer::requestLayout(layer);

    auto* childLayer = dynamic_cast<Layer*>(layer);
    if (childLayer != nullptr && childLayer != this && childLayer->getParent().get() == this) {
        mustGetFlexboxNode(childLayer)->setLayoutDirty();
    }
}

void FlexboxLayer::onChildRemoved(Layer* childLayer) {
    auto ownerNode = mustGetFlexboxNode(this);
    auto childNode = mustGetFlexboxNode(childLayer);

    YGNodeRemoveChild(ownerNode->yogaNode, childNode->yogaNode);
}

void FlexboxLayer::onChildInserted(Layer* childLayer, size_t index) {
    auto ownerNode = mustGetFlexboxNode(this);
    auto childNode = getOrCreateFlexboxNode(childLayer);

    auto* previousParent = YGNodeGetParent(childNode->yogaNode);
    if (previousParent != nullptr) {
        YGNodeRemoveChild(previousParent, childNode->yogaNode);
    }
    YGNodeInsertChild(ownerNode->yogaNode, childNode->yogaNode, static_cast<uint32_t>(index));
}

} // namespace snap::drawing
