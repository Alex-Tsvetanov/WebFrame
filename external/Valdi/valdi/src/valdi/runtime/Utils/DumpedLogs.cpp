//
//  DumpedLogs.cpp
//  valdi-pc
//
//  Created by Simon Corsin on 4/17/20.
//

#include "valdi/runtime/Utils/DumpedLogs.hpp"

namespace Valdi {

DumpedLogs::DumpedLogs() : _metadata(makeShared<ValueMap>()) {}

void DumpedLogs::appendVerbose(const StringBox& name, const Value& verbose) {
    DumpedVerboseLogs verboseLogs;
    verboseLogs.name = name;
    verboseLogs.data = verbose;

    _verbose.emplace_back(std::move(verboseLogs));
}

void DumpedLogs::appendMetadata(const StringBox& key, const StringBox& value) {
    (*_metadata)[key] = Value(value);
}

void DumpedLogs::append(const DumpedLogs& other) {
    for (const auto& verboseLogs : other.getVerbose()) {
        appendVerbose(verboseLogs.name, verboseLogs.data);
    }

    for (const auto& it : other.getMetadata()) {
        appendMetadata(it.first, it.second.toStringBox());
    }
}

const std::vector<DumpedVerboseLogs>& DumpedLogs::getVerbose() const {
    return _verbose;
}

const ValueMap& DumpedLogs::getMetadata() const {
    return *_metadata;
}

StringBox DumpedLogs::serializeMetadata() const {
    auto keys = Value(_metadata).sortedMapKeys();

    StringBox metadata;

    for (const auto& key : keys) {
        auto value = _metadata->find(key);

        metadata = metadata.append(key).append(" = ").append(value->second.toStringBox()).append("\n");
    }

    return metadata;
}

std::string DumpedLogs::verboseLogsToString() const {
    std::string output;

    auto first = true;
    for (const auto& verboseLogs : getVerbose()) {
        if (!first) {
            output += "\n\n";
        }
        first = false;
        output += verboseLogs.name.toStringView();
        output += ":";
        output += verboseLogs.data.toString(true);
    }

    return output;
}

} // namespace Valdi
