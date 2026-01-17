//
//  InMemoryDiskCache.hpp
//  valdi-pc
//
//  Created by Simon Corsin on 10/14/19.
//

#pragma once

#include "valdi/runtime/Interfaces/IDiskCache.hpp"

#include "valdi_core/cpp/Utils/Bytes.hpp"
#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include "valdi_core/cpp/Utils/Mutex.hpp"
#include "valdi_core/cpp/Utils/PathUtils.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"

#include <mutex>

namespace Valdi {

class InMemoryDiskCache : public IDiskCache {
public:
    struct Cache : public SimpleRefCountable {
        FlatMap<StringBox, BytesView> entries;
        Mutex mutex;
    };

    InMemoryDiskCache();
    InMemoryDiskCache(const Ref<Cache>& cache, const Path& rootPath);
    ~InMemoryDiskCache() override;
    bool exists(const Path& path) override;

    Result<BytesView> load(const Path& path) override;

    Result<BytesView> loadForAbsoluteURL(const StringBox& url) override;

    Result<Void> store(const Path& path, const BytesView& bytes) override;

    bool remove(const Path& path) override;

    StringBox getAbsoluteURL(const Path& path) const override;

    bool removeForAbsolutePath(const Path& absolutePath);

    Ref<IDiskCache> scopedCache(const Path& path, bool allowsReadOutsideOfScope) const override;

    void clear();

    Path getRootPath() const override;

    std::vector<Path> list(const Path& path) const override;

    FlatMap<StringBox, BytesView> getAll() const;

private:
    Path _rootPath;
    Ref<Cache> _cache;

    StringBox resolveFileKey(const Path& relativePath) const;
    Path toAbsolutePath(const Path& relativePath) const;
    Result<BytesView> getForAbsolutePath(const StringBox& absolutePath) const;
};

} // namespace Valdi
