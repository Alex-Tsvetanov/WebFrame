//
//  ValdiModuleArchive.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 3/6/19.
//

#pragma once

#include "valdi_core/cpp/Utils/Bytes.hpp"
#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"
#include <vector>

namespace Valdi {

struct ValdiModuleArchiveEntry {
    const Byte* data;
    size_t size;
};

class ValdiModuleArchive : public SharedPtrRefCountable {
public:
    ValdiModuleArchive();
    ~ValdiModuleArchive() override;

    std::optional<ValdiModuleArchiveEntry> getEntry(const Valdi::StringBox& path) const;
    ValdiModuleArchiveEntry getEntryForIndex(size_t index) const;

    bool containsEntry(const Valdi::StringBox& path) const;

    const std::vector<StringBox>& getAllEntryPaths() const;

    const BytesView& getDecompressedContent() const;

    [[nodiscard]] static Result<ValdiModuleArchive> decompress(const Byte* data, size_t len);
    [[nodiscard]] static Result<ValdiModuleArchive> deserialize(BytesView decompressedContent);

    bool operator==(const ValdiModuleArchive& other) const;
    bool operator!=(const ValdiModuleArchive& other) const;

private:
    BytesView _decompressedContent;
    FlatMap<StringBox, ValdiModuleArchiveEntry> _entries;
    std::vector<StringBox> _orderedEntryPaths;

    ValdiModuleArchive(BytesView decompressedContent,
                       FlatMap<StringBox, ValdiModuleArchiveEntry> entries,
                       std::vector<StringBox> orderedEntryPaths);
};

} // namespace Valdi
