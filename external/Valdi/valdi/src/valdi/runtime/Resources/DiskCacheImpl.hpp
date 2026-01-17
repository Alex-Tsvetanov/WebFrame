//
//  DiskCacheImpl.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 9/30/19.
//

#pragma once

#include "valdi/runtime/Interfaces/IDiskCache.hpp"
#include "valdi_core/cpp/Utils/PathUtils.hpp"
#include <string>

namespace Valdi {

/**
 A DiskCache implementation which writes to the disk absolute paths
 resolving using the given root path.
 */
class DiskCacheImpl : public IDiskCache {
public:
    explicit DiskCacheImpl(const StringBox& rootPath);
    DiskCacheImpl(Path rootPath, Path allowedReadPath);
    ~DiskCacheImpl() override;

    bool exists(const Path& path) override;

    Result<BytesView> load(const Path& path) override;

    Result<BytesView> loadForAbsoluteURL(const StringBox& url) override;

    Result<Void> store(const Path& path, const BytesView& bytes) override;

    bool remove(const Path& path) override;

    StringBox getAbsoluteURL(const Path& path) const override;

    Ref<IDiskCache> scopedCache(const Path& subfolder, bool allowsReadOutsideOfScope) const override;

    Path getRootPath() const override;

    std::vector<Path> list(const Path& path) const override;

    void setRootPath(const StringBox& rootPath);
    void setAllowedReadPath(const StringBox& allowedReadPath);

private:
    Path _rootPath;
    Path _allowedReadPath;

    Result<Path> resolveAbsolutePath(const Path& path, bool isRead) const;
};

} // namespace Valdi
