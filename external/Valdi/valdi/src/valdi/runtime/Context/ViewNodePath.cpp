//
//  ViewNodePath.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 8/24/18.
//

#include "valdi/runtime/Context/ViewNodePath.hpp"
#include "valdi_core/cpp/Attributes/AttributeUtils.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"
#include <fmt/format.h>

namespace Valdi {

ViewNodePathEntry::ViewNodePathEntry(StringBox nodeId, int itemIndex)
    : nodeId(std::move(nodeId)), itemIndex(itemIndex) {}

ViewNodePath::ViewNodePath(std::vector<ViewNodePathEntry> entries) : _entries(std::move(entries)) {}

Result<ViewNodePath> ViewNodePath::parse(const std::string_view& str) {
    std::vector<ViewNodePathEntry> entries;

    TextParser parser(str);

    while (!parser.isAtEnd()) {
        if (!entries.empty()) {
            if (!parser.tryParse('.') && !parser.tryParse(':')) {
                parser.setErrorAtCurrentPosition("Missing dot or colon separator");
                return parser.getError();
            }
        }

        auto identifier = parser.parseIdentifier();
        if (!identifier) {
            return parser.getError();
        }

        int itemIndex = 0;
        if (parser.tryParse('[')) {
            auto itemIndexResult = parser.parseInt();
            if (!itemIndexResult || !parser.parse(']')) {
                return parser.getError();
            }
            itemIndex = itemIndexResult.value();
        }

        entries.emplace_back(StringCache::getGlobal().makeString(identifier.value()), itemIndex);
    }

    return ViewNodePath(std::move(entries));
}

const std::vector<ViewNodePathEntry>& ViewNodePath::getEntries() const {
    return _entries;
}

} // namespace Valdi
