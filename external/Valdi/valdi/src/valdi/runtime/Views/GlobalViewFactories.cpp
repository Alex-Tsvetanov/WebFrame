//
//  GlobalViewFactories.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 1/28/20.
//

#include "valdi/runtime/Views/GlobalViewFactories.hpp"
#include "valdi/runtime/Attributes/AttributesManager.hpp"
#include "valdi/runtime/Interfaces/IViewManager.hpp"
#include "valdi/runtime/Views/ViewFactory.hpp"

namespace Valdi {

GlobalViewFactories::GlobalViewFactories(AttributesManager& attributesManager)
    : _attributesManager(attributesManager) {}

GlobalViewFactories::~GlobalViewFactories() = default;

void GlobalViewFactories::clearViewPools() {
    std::lock_guard<Mutex> guard(_mutex);

    for (const auto& it : _viewFactories) {
        it.second->clearViewPool();
    }
}

void GlobalViewFactories::registerViewClassReplacement(const StringBox& className,
                                                       const StringBox& replacementClassName) {
    std::lock_guard<Mutex> guard(_mutex);
    _viewClassReplacements[className] = replacementClassName;

    // Remove the associated view factory for the class we want to replace, so that next queries to
    // getViewFactory will resolve our replacement view factory.
    const auto& it = _viewFactories.find(className);
    if (it != _viewFactories.end()) {
        _viewFactories.erase(it);
    }
}

Ref<ViewFactory> GlobalViewFactories::getViewFactory(const StringBox& className) {
    std::unique_lock<Mutex> guard(_mutex);

    const auto& it = _viewFactories.find(className);
    if (it != _viewFactories.end()) {
        return it->second;
    }

    const auto& replacementIt = _viewClassReplacements.find(className);
    if (replacementIt != _viewClassReplacements.end()) {
        auto viewFactory = createViewFactory(replacementIt->second);

        _viewFactories[className] = viewFactory;
        _viewFactories[replacementIt->second] = viewFactory;

        return viewFactory;
    } else {
        // Call createViewFactory() with mutex released
        // This is because createViewFactory() calls attributesManager.getAttributesForClass()
        // which could lead the execution back to GlobalViewFactories
        guard.unlock();
        auto viewFactory = createViewFactory(className);
        guard.lock();
        // After re-acquiring the lock, we need to check if another thread has beat us to create
        // the factory and put it in the map
        auto it = _viewFactories.find(className);
        if (it != _viewFactories.end()) {
            return it->second;
        } else {
            _viewFactories[className] = viewFactory;
            return viewFactory;
        }
    }
}

Ref<ViewFactory> GlobalViewFactories::createViewFactory(const StringBox& className) {
    auto boundAttributes = _attributesManager.getAttributesForClass(className);
    return _attributesManager.getViewManager().createViewFactory(className, boundAttributes);
}

std::vector<Ref<ViewFactory>> GlobalViewFactories::copyViewFactories() const {
    std::vector<Ref<ViewFactory>> viewFactories;

    std::lock_guard<Mutex> guard(_mutex);
    viewFactories.reserve(_viewFactories.size());
    for (const auto& it : _viewFactories) {
        viewFactories.emplace_back(it.second);
    }

    return viewFactories;
}

} // namespace Valdi
