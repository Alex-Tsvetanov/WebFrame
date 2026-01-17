//
//  ViewPreloader.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 7/18/19.
//

#pragma once

#include "valdi/runtime/Interfaces/IViewManager.hpp"
#include "valdi/runtime/Views/GlobalViewFactories.hpp"
#include "valdi/runtime/Views/ViewFactory.hpp"
#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"
#include "valdi_core/cpp/Utils/ValdiObject.hpp"

#include <mutex>
#include <optional>
#include <vector>

namespace Valdi {

class MainThreadManager;
class IViewManager;
class ILogger;
class DispatchQueue;

class ViewPreloader : public ValdiObject {
public:
    ViewPreloader(Ref<GlobalViewFactories> viewFactories, MainThreadManager& mainThreadManager, ILogger& logger);
    ~ViewPreloader() override;

    void pausePreload();
    void resumePreload();

    void stopPreload();
    void startPreload(const StringBox& className, size_t count);

    void setWorkQueue(const Ref<DispatchQueue>& workQueue);

    VALDI_CLASS_HEADER(ViewPreloader)

private:
    Ref<GlobalViewFactories> _globalViewFactories;
    MainThreadManager& _mainThreadManager;
    [[maybe_unused]] ILogger& _logger;
    Ref<DispatchQueue> _workQueue;

    FlatMap<StringBox, size_t> _preloadCountByClassName;
    std::mutex _mutex;
    bool _preloading = false;
    bool _paused = false;

    void preload();
    bool preloadNext();
    void schedulePreload();
    void schedulePreloadIfNeeded();

    Ref<ViewFactory> dequeueNextViewFactoryToPreload();
};

} // namespace Valdi
