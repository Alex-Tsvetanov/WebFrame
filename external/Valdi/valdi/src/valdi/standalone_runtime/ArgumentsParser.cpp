//
//  ArgumentsParser.cpp
//  valdi-desktop-apple
//
//  Created by Simon Corsin on 2/9/22.
//

#include "valdi/standalone_runtime/ArgumentsParser.hpp"

#include "valdi_core/cpp/Utils/Format.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"

#include "utils/debugging/Assert.hpp"

namespace Valdi {

Argument::Argument(const StringBox& name) : _name(name) {}
Argument::~Argument() = default;

Ref<Argument> Argument::setDescription(std::string_view description) {
    _description = description;
    return strongSmallRef(this);
}

const std::string_view& Argument::getDescription() const {
    return _description;
}

Ref<Argument> Argument::setRequired() {
    _required = true;
    return strongSmallRef(this);
}

bool Argument::isRequired() const {
    return _required;
}

Ref<Argument> Argument::setChoices(std::vector<StringBox> choices) {
    _choices = std::move(choices);
    _hasChoices = true;
    return strongSmallRef(this);
}

Ref<Argument> Argument::setChoices(const std::string_view* choices, size_t length) {
    std::vector<StringBox> out;
    out.reserve(length);
    for (size_t i = 0; i < length; i++) {
        out.emplace_back(StringCache::getGlobal().makeString(choices[i]));
    }

    return setChoices(std::move(out));
}

Ref<Argument> Argument::setAsFlag() {
    _isFlag = true;
    return strongSmallRef(this);
}

Ref<Argument> Argument::setAsRemainder() {
    _isRemainder = true;
    return setAllowsMultipleValues();
}

bool Argument::hasChoices() const {
    return _hasChoices;
}

const std::vector<StringBox>& Argument::getChoices() const {
    return _choices;
}

size_t Argument::getChoiceIndex() const {
    return _choiceIndex;
}

void Argument::setChoiceIndex(size_t choiceIndex) {
    _choiceIndex = choiceIndex;
}

StringBox Argument::value() {
    if (_values.empty()) {
        return StringBox();
    }
    return _values[0];
}

bool Argument::hasValue() const {
    return !_values.empty();
}

const std::vector<StringBox>& Argument::values() const {
    return _values;
}

void Argument::appendValue(const StringBox& value) {
    _values.emplace_back(value);
}

Ref<Argument> Argument::setAllowsMultipleValues() {
    _allowsMultipleValues = true;
    return strongSmallRef(this);
}

bool Argument::allowsMultipleValues() const {
    return _allowsMultipleValues;
}

bool Argument::isFlag() const {
    return _isFlag;
}

bool Argument::isRemainder() const {
    return _isRemainder;
}

const StringBox& Argument::getName() const {
    return _name;
}

StringBox Argument::getFullDescription() const {
    if (_description.empty()) {
        return _name;
    }

    return STRING_FORMAT("{} {}", _name, _description);
}

ArgumentsParser::ArgumentsParser() = default;
ArgumentsParser::~ArgumentsParser() = default;

Ref<Argument> ArgumentsParser::addArgument(std::string_view name) {
    auto key = StringCache::getGlobal().makeString(name);
    auto argument = makeShared<Argument>(key);

    SC_ASSERT(getArgument(key) == nullptr);
    _argumentsByName[key] = argument;
    _arguments.emplace_back(argument);

    return argument;
}

Ref<Argument> ArgumentsParser::getArgument(const StringBox& name) const {
    const auto& it = _argumentsByName.find(name);
    if (it == _argumentsByName.end()) {
        return nullptr;
    }

    return it->second;
}

Result<Void> ArgumentsParser::parse(Arguments& arguments) {
    while (arguments.hasNext()) {
        auto argumentKey = arguments.next();
        auto argument = getArgument(argumentKey);
        if (argument == nullptr) {
            return Error(STRING_FORMAT("Unrecognized argument '{}'", argumentKey));
        }

        if (argument->isRemainder()) {
            while (arguments.hasNext()) {
                argument->appendValue(arguments.next());
            }
        } else if (argument->isFlag()) {
            if (!argument->hasValue()) {
                argument->appendValue(STRING_LITERAL("true"));
            }
        } else {
            if (!arguments.hasNext()) {
                return Error(STRING_FORMAT("Missing value after argument '{}'", argument->getFullDescription()));
            }

            auto argumentValue = arguments.next();

            if (argument->hasValue() && !argument->allowsMultipleValues()) {
                return Error(STRING_FORMAT("Argument '{}' can only accept one value", argument->getFullDescription()));
            }

            argument->appendValue(argumentValue);

            if (argument->hasChoices()) {
                std::optional<size_t> choiceIndex;

                for (size_t i = 0; i < argument->getChoices().size(); i++) {
                    const auto& choice = argument->getChoices()[i];
                    if (choice == argumentValue) {
                        choiceIndex = {i};
                        break;
                    }
                }

                if (choiceIndex) {
                    argument->setChoiceIndex(choiceIndex.value());
                } else {
                    std::vector<StringBox> debugChoices;
                    for (const auto& choice : argument->getChoices()) {
                        debugChoices.emplace_back(STRING_FORMAT("'{}'", choice));
                    }
                    return Error(STRING_FORMAT("Argument '{}' must be one of ",
                                               argument->getFullDescription(),
                                               StringBox::join(debugChoices, ", ")));
                }
            }
        }
    }

    for (const auto& argument : _arguments) {
        if (!argument->hasValue() && argument->isRequired()) {
            return Error(
                STRING_FORMAT("Argument '{}' is required and was not provided", argument->getFullDescription()));
        }
    }

    return Void();
}

StringBox ArgumentsParser::getFullDescription() const {
    std::vector<StringBox> argumentNames;
    std::vector<std::string_view> argumentDescriptions;
    size_t largestArgumentName = 0;

    for (const auto& argument : _arguments) {
        largestArgumentName = std::max(largestArgumentName, argument->getName().length());
        argumentNames.emplace_back(argument->getName());
        argumentDescriptions.emplace_back(argument->getDescription());
    }

    std::string output;

    for (size_t i = 0; i < _arguments.size(); i++) {
        const auto& argumentName = argumentNames[i];
        const auto& argumentDescription = argumentDescriptions[i];
        output += "\t";
        output += argumentName.toStringView();
        auto spacesToOutput = largestArgumentName - argumentName.length();
        output.append(spacesToOutput, ' ');

        if (!argumentDescription.empty()) {
            output += " ";
            output += argumentDescription;
        }

        output += "\n";
    }

    return StringCache::getGlobal().makeString(output);
}

std::string ArgumentsParser::getUsageString(std::string_view programName) const {
    return fmt::format("{}\n\nAvailable arguments:\n{}", programName, getFullDescription().slowToString());
}

} // namespace Valdi
