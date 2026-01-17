//
//  ResourceId.hpp
//  ValdiRuntime
//
//  Created by Brandon Francis on 7/30/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#pragma once

#include "valdi_core/cpp/Utils/Result.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"
#include <ostream>
#include <string>

namespace Valdi {

struct ResourceId {
    StringBox bundleName;
    StringBox resourcePath;

    explicit ResourceId(StringBox resourcePath);
    ResourceId(StringBox bundleName, StringBox resourcePath);

    std::string toAbsolutePath() const;

    bool operator==(const Valdi::ResourceId& other) const;
    bool operator!=(const Valdi::ResourceId& other) const;

    bool operator<(const Valdi::ResourceId& other) const;

    friend std::ostream& operator<<(std::ostream& os, const ResourceId& d) noexcept;
};

} // namespace Valdi

namespace std {

template<>
struct hash<Valdi::ResourceId> {
    std::size_t operator()(const Valdi::ResourceId& k) const;
};

} // namespace std
