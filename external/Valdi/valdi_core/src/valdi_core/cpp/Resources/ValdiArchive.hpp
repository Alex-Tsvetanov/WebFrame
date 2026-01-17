//
//  ValdiArchive.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 4/19/19.
//

#pragma once

#include "valdi_core/cpp/Utils/ByteBuffer.hpp"
#include "valdi_core/cpp/Utils/Bytes.hpp"
#include "valdi_core/cpp/Utils/Function.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"
#include <vector>

namespace Valdi {

struct ValdiArchiveEntry {
    StringBox filePath;
    const Byte* data;
    size_t dataLength;

    ValdiArchiveEntry(StringBox filePath, const Byte* data, size_t dataLength);
    ValdiArchiveEntry(StringBox filePath, const StringBox& stringData);

    StringBox getStringData() const;
};

class ValdiArchive {
public:
    explicit ValdiArchive(const Bytes& bytes);
    explicit ValdiArchive(const Byte* dataBegin, const Byte* dataEnd);

    [[nodiscard]] Result<std::vector<ValdiArchiveEntry>> getEntries() const;

private:
    const Byte* _dataBegin;
    const Byte* _dataEnd;
};

class ValdiArchiveBuilder {
public:
    ValdiArchiveBuilder();

    void addEntry(const ValdiArchiveEntry& entry);

    Ref<ByteBuffer> build() const;

private:
    ByteBuffer _moduleData;

    void appendPadding(size_t padding);
};

} // namespace Valdi
