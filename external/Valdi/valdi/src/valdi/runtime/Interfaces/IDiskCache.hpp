//
//  IDiskCache.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 9/30/19.
//

#pragma once

#include "valdi_core/cpp/Utils/Bytes.hpp"
#include "valdi_core/cpp/Utils/PathUtils.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"

namespace Valdi {

class IDiskCache : public SimpleRefCountable {
public:
    /**
     Returns whether an item exists at the given path
     */
    virtual bool exists(const Path& path) = 0;

    /**
     Load the content of the item at the given path and return the result
     as bytes.
     */
    [[nodiscard]] virtual Result<BytesView> load(const Path& path) = 0;

    /**
     Load the content of the item at the given absolute URL and return the result
     as bytes.
     */
    [[nodiscard]] virtual Result<BytesView> loadForAbsoluteURL(const StringBox& url) = 0;

    /**
     Store the item at the given path with the given bytes.
     */
    [[nodiscard]] virtual Result<Void> store(const Path& path, const BytesView& bytes) = 0;

    /**
     Creates a new IDiskCache that will operate relative to the given path.
     */
    virtual Ref<IDiskCache> scopedCache(const Path& path, bool allowsReadOutsideOfScope) const = 0;

    /**
     Returns a list of files and folders within the cache's root directory.
     */
    virtual std::vector<Path> list(const Path& path) const = 0;

    /**
     Remove the item at the given path.
     Returns whether the file was successfully removed.
     */
    virtual bool remove(const Path& path) = 0;

    /**
     Returns the  root path of the cache
     */
    virtual Path getRootPath() const = 0;

    /**
     Return an absolute URL for the given file path. The url can be used elsewhere in the app
     to point to the given path. URLs are only valid during an app session and
     should not be stored on disk.
     */
    virtual StringBox getAbsoluteURL(const Path& path) const = 0;
};

} // namespace Valdi
