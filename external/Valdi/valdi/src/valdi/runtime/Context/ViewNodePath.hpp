//
//  ViewNodePath.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 8/24/18.
//

#pragma once

#include "valdi_core/cpp/Utils/Result.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"
#include <vector>

namespace Valdi {

struct ViewNodePathEntry {
    StringBox nodeId;
    int itemIndex;

    ViewNodePathEntry(StringBox nodeId, int itemIndex);
};

class ViewNodePath {
public:
    explicit ViewNodePath(std::vector<ViewNodePathEntry> entries);

    const std::vector<ViewNodePathEntry>& getEntries() const;

    [[nodiscard]] static Result<ViewNodePath> parse(const std::string_view& str);

private:
    std::vector<ViewNodePathEntry> _entries;
};

} // namespace Valdi
