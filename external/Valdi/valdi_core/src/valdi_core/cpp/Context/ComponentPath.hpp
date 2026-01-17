//
//  ComponentPath.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 12/16/19.
//

#pragma once

#include "valdi_core/cpp/Resources/ResourceId.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"
#include <iostream>

namespace Valdi {

class ComponentPath {
public:
    ComponentPath() noexcept;
    ComponentPath(ResourceId resourceId, StringBox symbolName) noexcept;

    const ResourceId& getResourceId() const;
    const StringBox& getSymbolName() const;

    std::string toString() const;

    bool isEmpty() const;

    static ComponentPath parse(const StringBox& componentPathString);

    friend std::ostream& operator<<(std::ostream& os, const ComponentPath& d) noexcept;

private:
    ResourceId _resourceId;
    StringBox _symbolName;
};

} // namespace Valdi
