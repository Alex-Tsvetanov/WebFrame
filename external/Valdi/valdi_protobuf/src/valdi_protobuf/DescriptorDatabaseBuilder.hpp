// Copyright © 2025 Snap, Inc. All rights reserved.

#pragma once

#include "valdi_core/cpp/Utils/Bytes.hpp"
#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"

#include "valdi_protobuf/FullyQualifiedName.hpp"

#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor_database.h>

#include <vector>

namespace DescriptorIndex {
class DescriptorIndex;
}

namespace Valdi {
class ExceptionTracker;
}

namespace Valdi::Protobuf {

class FullyQualifiedName;
class Message;
class Raw;
class Field;
class MessagePool;

// This class is for building the data structures in DescriptorDatabase.  In
// most case, this will not be used at all because DescriptorDatabase's index
// data will be pre-computed. This class is the fallback for cases when the
// pre-computed index is not available.
class DescriptorDatabaseBuilder {
public:
    DescriptorDatabaseBuilder();

    bool addFileDescriptorSet(const BytesView& data, ExceptionTracker& exceptionTracker);

    bool addFileDescriptor(const BytesView& data, ExceptionTracker& exceptionTracker);

    bool parseAndAddFileDescriptorSet(const std::string& filename,
                                      std::string_view protoFileContent,
                                      ExceptionTracker& exceptionTracker);

    bool build(std::vector<BytesView>& retainedBuffers, DescriptorIndex::DescriptorIndex& descriptorIndex);

private:
    struct FileEntry {
        StringBox fileName;
        const void* data = nullptr;
        size_t length = 0;
    };

    struct Symbol {
        FullyQualifiedName fullName;
        size_t fileIndex = 0;
        const google::protobuf::Descriptor* descriptor = nullptr;
    };

    struct Package {
        FullyQualifiedName fullName;
        std::vector<size_t> symbolIndexes;
        std::vector<size_t> nestedPackageIndexes;
    };

    std::vector<BytesView> _retainedBuffers;
    std::vector<FileEntry> _files;
    std::vector<Symbol> _symbols;
    std::vector<Package> _packages;
    FlatMap<StringBox, size_t> _fileIndexByName;
    FlatMap<FullyQualifiedName, size_t> _symbolIndexByName;
    FlatMap<FullyQualifiedName, size_t> _packageIndexByName;

    bool addFileDescriptorInner(MessagePool& messagePool,
                                const Byte* data,
                                size_t length,
                                ExceptionTracker& exceptionTracker);

    bool addDescriptor(size_t fileIndex,
                       size_t packageIndex,
                       const FullyQualifiedName& packageName,
                       MessagePool& messagePool,
                       const Protobuf::Field& field,
                       ExceptionTracker& exceptionTracker);

    bool addSymbol(size_t fileIndex,
                   size_t packageIndex,
                   const FullyQualifiedName& name,
                   ExceptionTracker& exceptionTracker);

    bool addSymbolFromField(size_t fileIndex,
                            size_t packageIndex,
                            FullyQualifiedName& outputName,
                            Message& outMessage,
                            const Protobuf::Field& field,
                            ExceptionTracker& exceptionTracker);

    size_t getOrCreatePackageIndex(const FullyQualifiedName& name);
};

} // namespace Valdi::Protobuf
