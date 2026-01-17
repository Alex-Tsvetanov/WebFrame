//
//  ViewNodeTreeManager.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 10/23/18.
//

#include "valdi/runtime/Context/Context.hpp"
#include "valdi/runtime/Context/ViewNodeTree.hpp"
#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include <memory>
#include <mutex>

namespace Valdi {

class Runtime;
class ILogger;
class MainThreadManager;

enum class ViewNodeTreeThreadAffinity {
    /**
     * The ViewNodeTree is expected to be updated in the Main Thread.
     */
    MAIN_THREAD = 1,

    /**
     * The ViewNodeTree can be updated in any thread.
     * When scheduling updates on the ViewNodeTree, it will be called in the caller
     * thread immediately.
     */
    ANY = 2,
};

class ViewNodeTreeManager {
public:
    ViewNodeTreeManager(MainThreadManager& mainThreadManager, ILogger& logger);

    SharedViewNodeTree getViewNodeTreeForContextId(ContextId contextId) const;

    SharedViewNodeTree createViewNodeTreeForContext(const SharedContext& context,
                                                    ViewNodeTreeThreadAffinity threadAffinity);

    SharedViewNodeTree getOrCreateViewNodeTreeForContext(const SharedContext& context,
                                                         ViewNodeTreeThreadAffinity threadAffinity);

    void removeViewNodeTree(ViewNodeTree& viewNodeTree);
    void removeAllViewNodeTrees();

    void setRuntime(std::weak_ptr<Runtime> runtime);

    std::vector<SharedViewNodeTree> getAllRootViewNodeTrees() const;

    int getViewNodeTreesCount() const;

private:
    MainThreadManager& _mainThreadManager;
    [[maybe_unused]] ILogger& _logger;
    std::weak_ptr<Runtime> _runtime;
    mutable std::mutex _mutex;

    FlatMap<ContextId, SharedViewNodeTree> _trees;

    void removeViewNodeTreeLockFree(ViewNodeTree& viewNodeTree);

    SharedViewNodeTree lockFreeCreateViewNodeTreeForContext(const SharedContext& context,
                                                            ViewNodeTreeThreadAffinity threadAffinity);
};

} // namespace Valdi
