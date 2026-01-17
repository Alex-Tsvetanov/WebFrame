//
//  AssetKey.hpp
//  valdi
//
//  Created by Simon Corsin on 6/28/21.
//

#pragma once

#include "valdi/runtime/Resources/Bundle.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"

namespace Valdi {

class Bundle;

class AssetKey {
public:
    AssetKey(const Ref<Bundle>& bundle, const StringBox& path);
    explicit AssetKey(const StringBox& url);
    ~AssetKey();

    const Ref<Bundle>& getBundle() const;
    const StringBox& getPath() const;

    const StringBox& getUrl() const;

    StringBox toString() const;

    bool isURL() const;

    bool operator==(const AssetKey& other) const;
    bool operator!=(const AssetKey& other) const;

private:
    Ref<Bundle> _bundle;
    StringBox _pathOrUrl;
};

std::ostream& operator<<(std::ostream& os, const Valdi::AssetKey& k) noexcept;

} // namespace Valdi

namespace std {

template<>
struct hash<Valdi::AssetKey> {
    std::size_t operator()(const Valdi::AssetKey& k) const noexcept;
};

} // namespace std
