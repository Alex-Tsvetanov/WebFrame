//
//  ReferenceInfo.cpp
//  valdi
//
//  Created by Simon Corsin on 7/12/22.
//

#include "valdi_core/cpp/Utils/ReferenceInfo.hpp"
#include "valdi_core/cpp/Utils/Format.hpp"
#include "valdi_core/cpp/Utils/StaticVector.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"

#include <iostream>
#include <sstream>

namespace Valdi {

// Makes sure that the size of ReferenceInfo remains small
static_assert(sizeof(ReferenceInfo) ==
              (ReferenceInfo::kComponentsSize * sizeof(uint16_t)) + (ReferenceInfo::kNamesSize * sizeof(StringBox)));

ReferenceInfo::ReferenceInfo() = default;

ReferenceInfo::ReferenceInfo(Components&& components, Names&& names)
    : _components(std::move(components)), _names(std::move(names)) {}

ReferenceInfo::~ReferenceInfo() = default;

const StringBox& ReferenceInfo::getName() const {
    return getNameForComponent(top());
}

const ReferenceInfo::Component& ReferenceInfo::top() const {
    return _components.front();
}

ReferenceInfo::Component& ReferenceInfo::top() {
    return _components.front();
}

const StringBox& ReferenceInfo::getNameForComponent(const Component& component) const {
    if (component.hasAssignedName) {
        return _names[component.index];
    } else {
        return StringBox::emptyString();
    }
}

std::string ReferenceInfo::toString() const {
    std::ostringstream ss;
    outputToStream(ss);
    return ss.str();
}

StringBox ReferenceInfo::toFunctionIdentifier() const {
    auto it = rbegin();
    auto end = rend();

    std::string functionName;

    while (it != end) {
        const auto& component = *it;
        std::string componentStr;

        switch (component.type) {
            case Component::Type::Unknown:
                componentStr = "<anon>";
                break;
            case Component::Type::Property:
                if (component.hasAssignedName) {
                    componentStr = getNameForComponent(component).slowToString();
                } else {
                    componentStr = "<anon>";
                }
                break;
            case Component::Type::ReturnValue:
                componentStr = "<r>";
                break;
            case Component::Type::ParameterValue:
                componentStr = fmt::format("<p{}>", component.index);
                break;
            case Component::Type::ArrayItem:
                componentStr = fmt::format("[{}]", component.index);
                break;
        }

        if (functionName.empty()) {
            functionName = std::move(componentStr);
        } else {
            functionName.append(1, '.');
            functionName += componentStr;
        }
        it++;
    }

    if (functionName.empty()) {
        functionName = "<anon>";
    }

    return StringCache::getGlobal().makeString(std::move(functionName));
}

const ReferenceInfo::Component* ReferenceInfo::begin() const {
    return _components.data();
}

const ReferenceInfo::Component* ReferenceInfo::end() const {
    size_t endIndex = _components.size();
    while (endIndex > 0) {
        if (_components[endIndex - 1].type != ReferenceInfo::Component::Type::Unknown) {
            break;
        }

        endIndex--;
    }

    return &_components[endIndex];
}

std::reverse_iterator<const ReferenceInfo::Component*> ReferenceInfo::rbegin() const {
    return std::reverse_iterator<const Component*>(end());
}

std::reverse_iterator<const ReferenceInfo::Component*> ReferenceInfo::rend() const {
    return std::reverse_iterator<const Component*>(begin());
}

void ReferenceInfo::outputToStream(std::ostream& os) const {
    auto it = rbegin();
    auto end = rend();

    if (it == end) {
        os << "<unknown>";
        return;
    }

    bool previousIsPropertyLike = false;
    bool hasPrevious = false;
    while (it != end) {
        const auto& component = *it;

        switch (component.type) {
            case Component::Type::Unknown:
                os << "<unknown>";
                previousIsPropertyLike = false;
                break;
            case Component::Type::Property:
                if (previousIsPropertyLike) {
                    os << ".";
                }

                if (component.hasAssignedName) {
                    os << getNameForComponent(component);
                } else {
                    if (component.isFunction) {
                        os << "<unknown_function>";
                    } else {
                        os << "<unknown_property>";
                    }
                }
                previousIsPropertyLike = true;

                break;
            case Component::Type::ReturnValue:
                if (hasPrevious) {
                    os << " -> ";
                }

                os << "<return_value>";
                previousIsPropertyLike = true;
                break;
            case Component::Type::ParameterValue:
                if (hasPrevious) {
                    os << " -> ";
                }

                os << "<parameter " << static_cast<size_t>(component.index);
                os << ">";
                previousIsPropertyLike = true;
                break;
            case Component::Type::ArrayItem:
                if (!hasPrevious) {
                    os << "<array>";
                }

                os << "[";
                os << static_cast<size_t>(component.index);
                os << "]";

                previousIsPropertyLike = true;
                break;
        }

        if (component.isFunction) {
            os << "()";
        }

        hasPrevious = true;
        it++;
    }
}

ReferenceInfo ReferenceInfo::fromObjectNamed(std::string_view objectName) {
    auto internedObjectName = StringCache::getGlobal().makeString(objectName);
    return ReferenceInfoBuilder().withObject(internedObjectName).build();
}

ReferenceInfoBuilder::ReferenceInfoBuilder(const ReferenceInfo& referenceInfo) : _referenceInfo(&referenceInfo) {}
ReferenceInfoBuilder::ReferenceInfoBuilder() = default;

ReferenceInfoBuilder::ReferenceInfoBuilder(const ReferenceInfoBuilder& parent,
                                           Type type,
                                           const StringBox* name,
                                           size_t index)
    : _parent(&parent), _type(type), _name(name), _index(index) {}
ReferenceInfoBuilder::ReferenceInfoBuilder(const ReferenceInfoBuilder* parent,
                                           const ReferenceInfo* referenceInfo,
                                           Type type,
                                           const StringBox* name,
                                           size_t index,
                                           bool isFunction)
    : _parent(parent),
      _referenceInfo(referenceInfo),
      _type(type),
      _name(name),
      _index(index),
      _isFunction(isFunction) {}

ReferenceInfoBuilder ReferenceInfoBuilder::withObject(const StringBox& object) const {
    return ReferenceInfoBuilder(*this, Type::Object, &object, 0);
}

ReferenceInfoBuilder ReferenceInfoBuilder::withProperty(const StringBox& property) const {
    return ReferenceInfoBuilder(*this, Type::Property, &property, 0);
}

ReferenceInfoBuilder ReferenceInfoBuilder::withArrayIndex(size_t index) const {
    return ReferenceInfoBuilder(*this, Type::ArrayItem, nullptr, index);
}

ReferenceInfoBuilder ReferenceInfoBuilder::withParameter(size_t index) const {
    return ReferenceInfoBuilder(*this, Type::ParameterValue, nullptr, index);
}

ReferenceInfoBuilder ReferenceInfoBuilder::asFunction() const {
    return ReferenceInfoBuilder(_parent, _referenceInfo, _type, _name, _index, true);
}

ReferenceInfoBuilder ReferenceInfoBuilder::withReturnValue() const {
    return ReferenceInfoBuilder(*this, Type::ReturnValue, nullptr, 0);
}

using ComponentsVector = StaticVector<ReferenceInfo::Component, ReferenceInfo::kComponentsSize>;
using NamesVector = StaticVector<StringBox, ReferenceInfo::kNamesSize>;

static bool assignComponent(ComponentsVector& components,
                            NamesVector& names,
                            ReferenceInfo::Component::Type type,
                            size_t index,
                            const StringBox* name,
                            bool isFunction) {
    if (components.size() >= ComponentsVector::capacity()) {
        return false;
    }

    auto& outComponent = components.emplace_back();

    if (name != nullptr && !name->isEmpty()) {
        if (names.size() < NamesVector::capacity()) {
            outComponent.hasAssignedName = true;
            outComponent.index = static_cast<uint8_t>(names.size());
            names.emplace_back(*name);
        }
    } else {
        outComponent.index = static_cast<uint8_t>(index);
    }
    outComponent.type = type;
    outComponent.isFunction = isFunction;

    return true;
}

static void assignReferenceInfo(ComponentsVector& components,
                                NamesVector& names,
                                const ReferenceInfo& referenceInfo,
                                bool isFunction) {
    const auto* begin = referenceInfo.begin();
    const auto* end = referenceInfo.end();

    const auto* it = begin;

    bool first = true;
    while (it != end) {
        bool resolvedIsFunction = it->isFunction || (first && isFunction);
        first = false;
        if (!assignComponent(components,
                             names,
                             it->type,
                             static_cast<size_t>(it->index),
                             &referenceInfo.getNameForComponent(*it),
                             resolvedIsFunction)) {
            break;
        }
        it++;
    }
}

ReferenceInfo ReferenceInfoBuilder::build() const {
    ComponentsVector components;
    NamesVector names;

    const auto* current = this;
    bool shouldContinue = true;

    while (current != nullptr && shouldContinue) {
        switch (current->_type) {
            case Type::Root:
                if (current->_referenceInfo != nullptr) {
                    assignReferenceInfo(components, names, *current->_referenceInfo, current->_isFunction);
                }
                break;
            case Type::Object:
                shouldContinue = assignComponent(components,
                                                 names,
                                                 ReferenceInfo::Component::Type::Property,
                                                 current->_index,
                                                 current->_name,
                                                 current->_isFunction);
                break;
            case Type::Property:
                shouldContinue = assignComponent(components,
                                                 names,
                                                 ReferenceInfo::Component::Type::Property,
                                                 current->_index,
                                                 current->_name,
                                                 current->_isFunction);
                break;
            case Type::ReturnValue:
                shouldContinue = assignComponent(components,
                                                 names,
                                                 ReferenceInfo::Component::Type::ReturnValue,
                                                 current->_index,
                                                 current->_name,
                                                 current->_isFunction);
                break;
            case Type::ParameterValue:
                shouldContinue = assignComponent(components,
                                                 names,
                                                 ReferenceInfo::Component::Type::ParameterValue,
                                                 current->_index,
                                                 current->_name,
                                                 current->_isFunction);
                break;
            case Type::ArrayItem:
                shouldContinue = assignComponent(components,
                                                 names,
                                                 ReferenceInfo::Component::Type::ArrayItem,
                                                 current->_index,
                                                 current->_name,
                                                 current->_isFunction);
                break;
        }

        current = current->_parent;
    }

    ReferenceInfo::Components outComponents;
    ReferenceInfo::Names outNames;

    size_t i = 0;
    for (auto& name : names) {
        outNames[i++] = std::move(name);
    }

    i = 0;
    for (auto& component : components) {
        outComponents[i++] = std::move(component);
    }

    return ReferenceInfo(std::move(outComponents), std::move(outNames));
}

std::ostream& operator<<(std::ostream& os, const Valdi::ReferenceInfo& value) noexcept {
    value.outputToStream(os);
    return os;
}

} // namespace Valdi
