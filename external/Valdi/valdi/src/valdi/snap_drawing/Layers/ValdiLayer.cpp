//
//  ValdiLayer.cpp
//  valdi-desktop-apple
//
//  Created by Simon Corsin on 6/30/20.
//

#include "valdi/snap_drawing/Layers/ValdiLayer.hpp"
#include "valdi/snap_drawing/Utils/ValdiUtils.hpp"

#include "valdi/runtime/Context/ViewManagerContext.hpp"
#include "valdi/runtime/Runtime.hpp"

#include "valdi_core/cpp/Utils/LazyValueConvertible.hpp"

#include "valdi_core/cpp/Utils/LoggerUtils.hpp"

namespace snap::drawing {

ValdiLayer::ValdiLayer(const Ref<Resources>& resources,
                       Valdi::Runtime* runtime,
                       Valdi::ViewManagerContext* viewManagerContext)
    : Layer(resources), _runtime(runtime), _viewManagerContext(viewManagerContext) {}

ValdiLayer::~ValdiLayer() {
    destroyComponent();
}

static Valdi::Shared<Valdi::ValueConvertible> toLazyValueConvertible(const Valdi::Value& value) {
    return Valdi::makeShared<Valdi::LazyValueConvertible>([value]() { return value; });
}

void ValdiLayer::setComponent(const Valdi::StringBox& componentPath,
                              const Valdi::Value& viewModel,
                              const Valdi::Value& componentContext) {
    setComponent(componentPath, toLazyValueConvertible(viewModel), toLazyValueConvertible(componentContext));
}

void ValdiLayer::setComponent(const Valdi::StringBox& componentPath,
                              const Valdi::Shared<Valdi::ValueConvertible>& viewModel,
                              const Valdi::Shared<Valdi::ValueConvertible>& componentContext) {
    destroyComponent();

    auto rootView = layerToValdiView(Valdi::strongSmallRef(this), false);

    auto viewNodeTree =
        _runtime->createViewNodeTreeAndContext(_viewManagerContext, componentPath, viewModel, componentContext);
    _valdiContext = viewNodeTree->getContext();

    viewNodeTree->setRootView(rootView);

    setNeedsLayout();
}

void ValdiLayer::onLayout() {
    updateLayoutSpecs();
}

void ValdiLayer::onBoundsChanged() {
    updateLayoutSpecs();
}

void ValdiLayer::setViewModel(const Valdi::Value& viewModel) {
    if (_valdiContext != nullptr) {
        _valdiContext->setViewModel(toLazyValueConvertible(viewModel));
    }
}

void ValdiLayer::destroyComponent() {
    if (_valdiContext != nullptr) {
        _runtime->destroyContext(_valdiContext);
        _valdiContext = nullptr;
    }
}

Size ValdiLayer::sizeThatFits(Size maxSize) {
    return calculateLayout(maxSize);
}

void ValdiLayer::onRightToLeftChanged() {
    Layer::onRightToLeftChanged();

    updateLayoutSpecs();
}

void ValdiLayer::updateLayoutSpecs() {
    if (_valdiContext == nullptr) {
        return;
    }

    auto viewNodeTree = _runtime->getViewNodeTreeManager().getViewNodeTreeForContextId(_valdiContext->getContextId());

    if (viewNodeTree == nullptr) {
        return;
    }

    viewNodeTree->setLayoutSpecs(Valdi::Size(getFrame().width(), getFrame().height()),
                                 isRightToLeft() ? Valdi::LayoutDirectionRTL : Valdi::LayoutDirectionLTR);
}

Size ValdiLayer::calculateLayout(Size maxSize) {
    if (_valdiContext == nullptr) {
        return Size::makeEmpty();
    }

    auto viewNodeTree = _runtime->getViewNodeTreeManager().getViewNodeTreeForContextId(_valdiContext->getContextId());
    if (viewNodeTree == nullptr) {
        return Size::makeEmpty();
    }
    auto lock = viewNodeTree->lock();

    auto measuredSize =
        viewNodeTree->measureLayout(maxSize.width,
                                    Valdi::MeasureMode::MeasureModeAtMost,
                                    maxSize.height,
                                    Valdi::MeasureMode::MeasureModeAtMost,
                                    isRightToLeft() ? Valdi::LayoutDirectionRTL : Valdi::LayoutDirectionLTR);

    return Size::make(measuredSize.width, measuredSize.height);
}

const Valdi::Ref<Valdi::Context>& ValdiLayer::getValdiContext() const {
    return _valdiContext;
}

} // namespace snap::drawing
