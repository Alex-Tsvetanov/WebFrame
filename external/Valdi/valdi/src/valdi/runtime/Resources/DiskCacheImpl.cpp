//
//  DiskCacheImpl.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 9/30/19.
//

#include "valdi/runtime/Resources/DiskCacheImpl.hpp"
#include "valdi_core/cpp/Utils/DiskUtils.hpp"
#include "valdi_core/cpp/Utils/Format.hpp"
#include "valdi_core/cpp/Utils/PathUtils.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"
#include "valdi_core/cpp/Utils/URL.hpp"

namespace Valdi {

DiskCacheImpl::DiskCacheImpl(const StringBox& rootPath)
    : _rootPath(rootPath.toStringView()), _allowedReadPath(_rootPath) {
    _rootPath.normalize();
    _allowedReadPath.normalize();
}

DiskCacheImpl::DiskCacheImpl(Path rootPath, Path allowedReadPath)
    : _rootPath(std::move(rootPath)), _allowedReadPath(std::move(allowedReadPath)) {
    _rootPath.normalize();
    _allowedReadPath.normalize();
}

DiskCacheImpl::~DiskCacheImpl() = default;

bool DiskCacheImpl::exists(const Path& path) {
    auto resolvedPath = resolveAbsolutePath(path, true);
    if (!resolvedPath) {
        return false;
    }

    return DiskUtils::isFile(resolvedPath.value());
}

Result<BytesView> DiskCacheImpl::load(const Path& path) {
    auto resolvedPath = resolveAbsolutePath(path, true);
    if (!resolvedPath) {
        return resolvedPath.moveError();
    }

    return DiskUtils::load(resolvedPath.value());
}

Result<BytesView> DiskCacheImpl::loadForAbsoluteURL(const StringBox& url) {
    URL parsedURL(url);
    if (parsedURL.getScheme() != "file") {
        return Error("Invalid url");
    }

    Path path(parsedURL.getPath().toStringView());
    return load(path);
}

Result<Void> DiskCacheImpl::store(const Path& path, const BytesView& bytes) {
    auto resolvedPath = resolveAbsolutePath(path, false);
    if (!resolvedPath) {
        return resolvedPath.moveError();
    }

    auto filePath = resolvedPath.moveValue();

    if (filePath.getComponents().size() > 1) {
        auto directoryPath = filePath.removingLastComponent();
        if (!DiskUtils::isDirectory(directoryPath) && !DiskUtils::makeDirectory(directoryPath, true)) {
            return Error(STRING_FORMAT("Failed to create directories to store file at {}", filePath.toString()));
        }
    }

    return DiskUtils::store(filePath, bytes);
}

Ref<IDiskCache> DiskCacheImpl::scopedCache(const Path& subfolder, bool allowsReadOutsideOfScope) const {
    auto result = resolveAbsolutePath(subfolder, false);
    if (result.failure()) {
        return nullptr;
    }
    auto rootPath = result.value();
    auto readPath = allowsReadOutsideOfScope ? _allowedReadPath : rootPath;

    auto scopedInstance = Valdi::makeShared<DiskCacheImpl>(std::move(rootPath), std::move(readPath));

    return scopedInstance;
}

bool DiskCacheImpl::remove(const Path& path) {
    auto resolvedPath = resolveAbsolutePath(path, false);
    if (!resolvedPath) {
        return false;
    }
    return DiskUtils::remove(resolvedPath.value());
}

StringBox DiskCacheImpl::getAbsoluteURL(const Path& path) const {
    auto resolvedPath = resolveAbsolutePath(path, false);
    if (!resolvedPath) {
        return StringCache::getGlobal().makeString(path.toString());
    }

    return STRING_FORMAT("file://{}", resolvedPath.value().toString());
}

Result<Path> DiskCacheImpl::resolveAbsolutePath(const Path& path, bool isRead) const {
    auto newPath = !path.isAbsolute() ? _rootPath.appending(path) : path;
    newPath.normalize();

    const auto& pathToCheck = isRead ? _allowedReadPath : _rootPath;

    if (!newPath.startsWith(pathToCheck)) {
        return Error(STRING_FORMAT(
            "Resolved path '{}' is outside of allowed path '{}'", newPath.toString(), pathToCheck.toString()));
    }

    return newPath;
}

std::vector<Path> DiskCacheImpl::list(const Path& path) const {
    return DiskUtils::listDirectory(path);
}

Path DiskCacheImpl::getRootPath() const {
    return _rootPath;
}

void DiskCacheImpl::setRootPath(const StringBox& rootPath) {
    _rootPath = Path(rootPath.toStringView());
    _rootPath.normalize();
}

void DiskCacheImpl::setAllowedReadPath(const StringBox& allowedReadPath) {
    _allowedReadPath = Path(allowedReadPath.toStringView());
    _allowedReadPath.normalize();
}

} // namespace Valdi
