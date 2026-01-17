//
//  DumpedLogs.hpp
//  valdi-pc
//
//  Created by Simon Corsin on 4/17/20.
//

#pragma once

#include "valdi_core/cpp/Utils/StringBox.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"
#include "valdi_core/cpp/Utils/ValueMap.hpp"

#include <vector>

namespace Valdi {

struct DumpedVerboseLogs {
    StringBox name;
    Value data;
};

class DumpedLogs {
public:
    DumpedLogs();

    void appendVerbose(const StringBox& name, const Value& verbose);
    void appendMetadata(const StringBox& key, const StringBox& value);

    void append(const DumpedLogs& other);

    const std::vector<DumpedVerboseLogs>& getVerbose() const;
    const ValueMap& getMetadata() const;

    StringBox serializeMetadata() const;

    std::string verboseLogsToString() const;

private:
    // Verbose logs, more suited to be written in a dedicated files or printed on the console
    std::vector<DumpedVerboseLogs> _verbose;
    // Metadata as key value pairs.
    Ref<ValueMap> _metadata;
};

} // namespace Valdi
