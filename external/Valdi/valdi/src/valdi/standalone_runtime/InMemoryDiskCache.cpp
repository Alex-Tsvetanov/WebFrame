//
//  InMemoryDiskCache.cpp
//  valdi-pc
//
//  Created by Simon Corsin on 10/14/19.
//

#include "valdi/standalone_runtime/InMemoryDiskCache.hpp"
#include "valdi_core/cpp/Utils/FlatSet.hpp"
#include "valdi_core/cpp/Utils/Format.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"
#include "valdi_core/cpp/Utils/URL.hpp"

#include <iostream>

namespace Valdi {

InMemoryDiskCache::InMemoryDiskCache() : InMemoryDiskCache(makeShared<InMemoryDiskCache::Cache>(), Path("/")) {}
InMemoryDiskCache::InMemoryDiskCache(const Ref<InMemoryDiskCache::Cache>& cache, const Path& rootPath)
    : _rootPath(rootPath), _cache(cache) {}

InMemoryDiskCache::~InMemoryDiskCache() = default;

StringBox InMemoryDiskCache::getAbsoluteURL(const Path& path) const {
    return STRING_FORMAT("file://{}", toAbsolutePath(path).toString());
}

Result<BytesView> InMemoryDiskCache::load(const Path& path) {
    return getForAbsolutePath(resolveFileKey(path));
}

Path InMemoryDiskCache::getRootPath() const {
    return _rootPath;
}

std::vector<Path> InMemoryDiskCache::list(const Path& path) const {
    auto absolutePath = toAbsolutePath(path);
    std::vector<Path> output;
    FlatSet<StringBox> seenEntries;
    std::lock_guard<Mutex> guard(_cache->mutex);

    auto& entries = _cache->entries;

    for (const auto& it : entries) {
        Path existingPath(it.first.toStringView());

        if (existingPath.getComponents().size() > absolutePath.getComponents().size() &&
            existingPath.startsWith(absolutePath)) {
            auto newPath = absolutePath.appending(existingPath.getComponents()[absolutePath.getComponents().size()]);
            auto inserted = seenEntries.insert(newPath.toStringBox());
            if (inserted.second) {
                output.emplace_back(std::move(newPath));
            }
        }
    }

    return output;
}

Path InMemoryDiskCache::toAbsolutePath(const Path& relativePath) const {
    auto newPath = !relativePath.isAbsolute() ? _rootPath.appending(relativePath) : relativePath;
    return newPath.normalize();
}

StringBox InMemoryDiskCache::resolveFileKey(const Path& relativePath) const {
    return toAbsolutePath(relativePath).toStringBox();
}

Result<BytesView> InMemoryDiskCache::loadForAbsoluteURL(const StringBox& url) {
    URL parsedURL(url);
    if (parsedURL.getScheme() != "file") {
        return Error("Invalid url");
    }

    return getForAbsolutePath(parsedURL.getPath());
}

Result<BytesView> InMemoryDiskCache::getForAbsolutePath(const StringBox& path) const {
    std::lock_guard<Mutex> guard(_cache->mutex);

    const auto it = _cache->entries.find(path);
    if (it == _cache->entries.end()) {
        return Error(STRING_FORMAT("No file at {}", path));
    }
    return it->second;
}

Result<Void> InMemoryDiskCache::store(const Path& path, const BytesView& bytes) {
    auto fileKey = resolveFileKey(path);

    std::lock_guard<Mutex> guard(_cache->mutex);
    _cache->entries[fileKey] = bytes;

    return Void();
}

bool InMemoryDiskCache::exists(const Path& path) {
    auto fileKey = resolveFileKey(path);
    std::lock_guard<Mutex> guard(_cache->mutex);
    return _cache->entries.find(fileKey) != _cache->entries.end();
}

bool InMemoryDiskCache::remove(const Path& path) {
    return removeForAbsolutePath(toAbsolutePath(path));
}

Ref<IDiskCache> InMemoryDiskCache::scopedCache(const Path& subfolder, bool /*allowsReadOutsideOfScope*/) const {
    return Valdi::makeShared<InMemoryDiskCache>(_cache, toAbsolutePath(subfolder));
}

bool InMemoryDiskCache::removeForAbsolutePath(const Path& absolutePath) {
    auto removed = false;

    std::lock_guard<Mutex> guard(_cache->mutex);

    auto& entries = _cache->entries;
    auto it = entries.begin();
    while (it != entries.end()) {
        Path existingPath(it->first.toStringView());

        if (existingPath.startsWith(absolutePath)) {
            it = entries.erase(it);
            removed = true;
        } else {
            it++;
        }
    }
    return removed;
}

void InMemoryDiskCache::clear() {
    std::lock_guard<Mutex> guard(_cache->mutex);
    _cache->entries.clear();
}

FlatMap<StringBox, BytesView> InMemoryDiskCache::getAll() const {
    std::lock_guard<Mutex> guard(_cache->mutex);
    return _cache->entries;
}

} // namespace Valdi
