//
//  ReferenceInfo.hpp
//  valdi
//
//  Created by Simon Corsin on 7/12/22.
//

#pragma once

#include "utils/base/NonCopyable.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"

#include <array>
#include <ostream>
#include <string>

namespace Valdi {

class StringBox;
class ReferenceInfoBuilder;

class ReferenceInfo {
public:
    static constexpr size_t kComponentsSize = 4;
    static constexpr size_t kNamesSize = 2;

    struct Component {
        enum class Type : uint8_t { Unknown, Property, ReturnValue, ParameterValue, ArrayItem };

        // NOLINTNEXTLINE(modernize-use-default-member-init)
        Type type;
        // NOLINTNEXTLINE(modernize-use-default-member-init)
        uint8_t index : 6;
        // NOLINTNEXTLINE(modernize-use-default-member-init)
        bool hasAssignedName : 1;
        // NOLINTNEXTLINE(modernize-use-default-member-init)
        bool isFunction : 1;

        constexpr Component() : type(Type::Unknown), index(0), hasAssignedName(false), isFunction(false) {}
    };

    ReferenceInfo();
    ~ReferenceInfo();

    const StringBox& getName() const;

    std::string toString() const;
    void outputToStream(std::ostream& os) const;

    StringBox toFunctionIdentifier() const;

    const Component* begin() const;
    const Component* end() const;

    std::reverse_iterator<const Component*> rbegin() const;
    std::reverse_iterator<const Component*> rend() const;

    const StringBox& getNameForComponent(const Component& component) const;

    static ReferenceInfo fromObjectNamed(std::string_view objectName);

private:
    friend ReferenceInfoBuilder;

    using Components = std::array<Component, kComponentsSize>;
    using Names = std::array<StringBox, kNamesSize>;

    Components _components;
    Names _names;

    ReferenceInfo(Components&& components, Names&& names);

    const Component& top() const;
    Component& top();
};

class ReferenceInfoBuilder : public snap::NonCopyable {
public:
    explicit ReferenceInfoBuilder(const ReferenceInfo& referenceInfo);
    ReferenceInfoBuilder();

    ReferenceInfoBuilder withObject(const StringBox& object) const;
    ReferenceInfoBuilder withProperty(const StringBox& property) const;
    ReferenceInfoBuilder withArrayIndex(size_t index) const;
    ReferenceInfoBuilder withParameter(size_t index) const;

    ReferenceInfoBuilder asFunction() const;

    ReferenceInfoBuilder withReturnValue() const;

    ReferenceInfo build() const;

private:
    enum class Type : uint8_t { Root, Object, Property, ReturnValue, ParameterValue, ArrayItem };

    const ReferenceInfoBuilder* _parent = nullptr;
    const ReferenceInfo* _referenceInfo = nullptr;
    Type _type = Type::Root;
    const StringBox* _name = nullptr;
    size_t _index = 0;
    bool _isFunction = false;

    ReferenceInfoBuilder(const ReferenceInfoBuilder* parent,
                         const ReferenceInfo* referenceInfo,
                         Type type,
                         const StringBox* name,
                         size_t index,
                         bool isFunction);
    ReferenceInfoBuilder(const ReferenceInfoBuilder& parent, Type type, const StringBox* name, size_t index);
};

std::ostream& operator<<(std::ostream& os, const Valdi::ReferenceInfo& value) noexcept;

} // namespace Valdi
