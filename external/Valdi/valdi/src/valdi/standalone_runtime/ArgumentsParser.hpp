//
//  ArgumentsParser.hpp
//  valdi-desktop-apple
//
//  Created by Simon Corsin on 2/9/22.
//

#pragma once

#include "valdi_core/cpp/Utils/StringBox.hpp"

#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"

#include "valdi/standalone_runtime/Arguments.hpp"

#include <vector>

namespace Valdi {

class Argument : public SimpleRefCountable {
public:
    Argument(const StringBox& name);
    ~Argument() override;

    Ref<Argument> setDescription(std::string_view description);
    const std::string_view& getDescription() const;

    Ref<Argument> setChoices(std::vector<StringBox> choices);

    Ref<Argument> setChoices(const std::string_view* choices, size_t length);

    Ref<Argument> setChoices(std::initializer_list<std::string_view> choices) {
        return setChoices(choices.begin(), choices.size());
    }

    Ref<Argument> setAsFlag();

    Ref<Argument> setAsRemainder();

    bool hasChoices() const;
    const std::vector<StringBox>& getChoices() const;

    Ref<Argument> setRequired();
    bool isRequired() const;

    StringBox value();
    const std::vector<StringBox>& values() const;
    bool hasValue() const;

    size_t getChoiceIndex() const;
    void setChoiceIndex(size_t choiceIndex);

    void appendValue(const StringBox& value);

    Ref<Argument> setAllowsMultipleValues();

    bool allowsMultipleValues() const;
    bool isFlag() const;
    bool isRemainder() const;

    const StringBox& getName() const;

    StringBox getFullDescription() const;

private:
    StringBox _name;
    std::string_view _description;
    std::vector<StringBox> _values;
    std::vector<StringBox> _choices;
    size_t _choiceIndex = 0;
    bool _required = false;
    bool _allowsMultipleValues = false;
    bool _hasChoices = false;
    bool _isFlag = false;
    bool _isRemainder = false;
};

class ArgumentsParser {
public:
    ArgumentsParser();
    ~ArgumentsParser();

    Ref<Argument> addArgument(std::string_view name);

    Result<Void> parse(Arguments& arguments);

    StringBox getFullDescription() const;

    std::string getUsageString(std::string_view programName) const;

private:
    std::vector<Ref<Argument>> _arguments;
    FlatMap<StringBox, Ref<Argument>> _argumentsByName;

    Ref<Argument> getArgument(const StringBox& name) const;
};

} // namespace Valdi
