//
//  StandaloneResourceLoader.hpp
//  valdi-pc
//
//  Created by Simon Corsin on 10/14/19.
//

#pragma once

#include "valdi/runtime/Interfaces/IResourceLoader.hpp"
#include "valdi_core/cpp/Resources/ResourceId.hpp"
#include "valdi_core/cpp/Resources/StaticResourceRegistry.hpp"
#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include "valdi_core/cpp/Utils/Mutex.hpp"
#include "valdi_core/cpp/Utils/PathUtils.hpp"

namespace Valdi {

class InMemoryDiskCache;

class StandaloneResourceLoader : public IResourceLoader {
public:
    StandaloneResourceLoader();
    ~StandaloneResourceLoader() override;

    Result<BytesView> loadModuleContent(const StringBox& module) override;

    StringBox resolveLocalAssetURL(const StringBox& moduleName, const StringBox& resourcePath) override;

    void setLocalAssetURL(const StringBox& moduleName, const StringBox& resourcePath, const StringBox& url);

    void setDiskCache(const Ref<InMemoryDiskCache>& diskCache);

    void addModuleSearchDirectory(const StringBox& searchDirectory);

private:
    FlatMap<ResourceId, StringBox> _localAssetURLs;
    FlatMap<StringBox, Path> _pathByModule;
    FlatMap<StringBox, Valdi::StaticResourceRegistry::Resource> _staticResourceByModule;
    std::vector<StringBox> _moduleSearchDirectories;
    Ref<InMemoryDiskCache> _diskCache;
    Mutex _mutex;

    Result<BytesView> searchForModule(const Path& directory, const StringBox& module);
};

} // namespace Valdi
