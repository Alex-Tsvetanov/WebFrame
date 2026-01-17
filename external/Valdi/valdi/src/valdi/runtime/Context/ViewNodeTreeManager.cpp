//
//  ViewNodeTreeManager.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 10/23/18.
//

#include "valdi/runtime/Context/ViewNodeTreeManager.hpp"
#include "valdi/runtime/Context/ViewManagerContext.hpp"

namespace Valdi {

ViewNodeTreeManager::ViewNodeTreeManager(MainThreadManager& mainThreadManager, ILogger& logger)
    : _mainThreadManager(mainThreadManager), _logger(logger) {}

SharedViewNodeTree ViewNodeTreeManager::getViewNodeTreeForContextId(ContextId contextId) const {
    auto lock = std::unique_lock<std::mutex>(_mutex);
    const auto& it = _trees.find(contextId);
    if (it == _trees.end()) {
        return nullptr;
    }
    return it->second;
}

SharedViewNodeTree ViewNodeTreeManager::createViewNodeTreeForContext(const SharedContext& context,
                                                                     ViewNodeTreeThreadAffinity threadAffinity) {
    auto lock = std::unique_lock<std::mutex>(_mutex);
    return lockFreeCreateViewNodeTreeForContext(context, threadAffinity);
}

SharedViewNodeTree ViewNodeTreeManager::lockFreeCreateViewNodeTreeForContext(
    const SharedContext& context, ViewNodeTreeThreadAffinity threadAffinity) {
    auto runtime = _runtime.lock();
    SC_ASSERT_NOTNULL(runtime);
    IViewManager* viewManager = nullptr;
    const auto& viewManagerContext = context->getViewManagerContext();
    if (viewManagerContext != nullptr) {
        viewManager = &viewManagerContext->getViewManager();
    }
    auto viewNodeTree = Valdi::makeShared<ViewNodeTree>(context,
                                                        viewManagerContext,
                                                        viewManager,
                                                        std::move(runtime),
                                                        &_mainThreadManager,
                                                        threadAffinity == ViewNodeTreeThreadAffinity::MAIN_THREAD);

    auto emplaced = _trees.try_emplace(context->getContextId(), viewNodeTree).second;
    SC_ASSERT(emplaced, "ViewNodeTree was already registered");

    return viewNodeTree;
}

SharedViewNodeTree ViewNodeTreeManager::getOrCreateViewNodeTreeForContext(const SharedContext& context,
                                                                          ViewNodeTreeThreadAffinity threadAffinity) {
    std::lock_guard<std::mutex> guard(_mutex);
    const auto& it = _trees.find(context->getContextId());
    if (it != _trees.end()) {
        return it->second;
    }

    return lockFreeCreateViewNodeTreeForContext(context, threadAffinity);
}

void ViewNodeTreeManager::removeViewNodeTree(ViewNodeTree& viewNodeTree) {
    auto lock = std::unique_lock<std::mutex>(_mutex);
    removeViewNodeTreeLockFree(viewNodeTree);
}

void ViewNodeTreeManager::removeViewNodeTreeLockFree(ViewNodeTree& viewNodeTree) {
    _trees.erase(viewNodeTree.getContext()->getContextId());
}

void ViewNodeTreeManager::removeAllViewNodeTrees() {
    auto lock = std::unique_lock<std::mutex>(_mutex);
    while (!_trees.empty()) {
        auto viewNodeTree = _trees.begin()->second;
        removeViewNodeTreeLockFree(*viewNodeTree);
    }
}

int ViewNodeTreeManager::getViewNodeTreesCount() const {
    auto lock = std::unique_lock<std::mutex>(_mutex);
    return static_cast<int>(_trees.size());
}

void ViewNodeTreeManager::setRuntime(std::weak_ptr<Runtime> runtime) {
    auto lock = std::unique_lock<std::mutex>(_mutex);
    _runtime = std::move(runtime);
}

std::vector<SharedViewNodeTree> ViewNodeTreeManager::getAllRootViewNodeTrees() const {
    auto lock = std::unique_lock<std::mutex>(_mutex);
    std::vector<SharedViewNodeTree> out;
    for (const auto& it : _trees) {
        out.emplace_back(it.second);
    }

    return out;
}

} // namespace Valdi
