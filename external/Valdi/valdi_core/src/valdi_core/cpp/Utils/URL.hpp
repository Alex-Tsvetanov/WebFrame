//
//  URL.hpp
//  valdi_core-pc
//
//  Created by Simon Corsin on 7/12/21.
//

#pragma once

#include "valdi_core/cpp/Utils/StringBox.hpp"

namespace Valdi {

class URL {
public:
    explicit URL(const StringBox& url);
    URL(StringBox&& scheme, StringBox&& path);
    ~URL();

    const StringBox& getScheme() const;
    const StringBox& getUrl() const;
    const StringBox& getPath() const;

    bool operator==(const URL& other) const;
    bool operator!=(const URL& other) const;

    URL withLastPathComponentRemoved() const;

private:
    StringBox _scheme;
    StringBox _url;
    StringBox _path;
};
} // namespace Valdi
