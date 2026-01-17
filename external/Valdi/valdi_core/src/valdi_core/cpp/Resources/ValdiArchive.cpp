//
//  ValdiArchive.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 4/19/19.
//

#include "valdi_core/cpp/Resources/ValdiArchive.hpp"
#include "valdi_core/cpp/Resources/ValdiPacket.hpp"
#include "valdi_core/cpp/Utils/ByteBuffer.hpp"
#include "valdi_core/cpp/Utils/InlineContainerAllocator.hpp"
#include "valdi_core/cpp/Utils/Parser.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"

namespace Valdi {

constexpr uint32_t kPaddingBit = 0x80000000;

struct LengthField {
    uint32_t raw;

    size_t size() const {
        // Keep lower 31 bits
        return static_cast<size_t>(raw & (kPaddingBit - 1));
    }

    bool hasPadding() const {
        // Look  at the top bit
        return (raw & kPaddingBit) != 0;
    }
};

static size_t computePadding(size_t size) {
    return alignUp(size, sizeof(uint32_t)) - size;
}

ValdiArchive::ValdiArchive(const Bytes& bytes) : ValdiArchive(bytes.data(), bytes.data() + bytes.size()) {}

ValdiArchive::ValdiArchive(const Byte* dataBegin, const Byte* dataEnd) : _dataBegin(dataBegin), _dataEnd(dataEnd) {}

Error onModuleFailure(Error&& error) {
    return error.rethrow("Invalid Valdi Archive");
}

static Result<BytesView> parseField(Parser<Byte>& parser, const LengthField& length) {
    auto size = length.size();
    auto fieldData = parser.parse<const Byte>(size);
    if (!fieldData) {
        return onModuleFailure(fieldData.moveError());
    }

    if (length.hasPadding()) {
        auto padding = computePadding(size);
        auto skipped = parser.parse<Void>(padding);
        if (!skipped) {
            return skipped.moveError();
        }
    }

    return BytesView(nullptr, fieldData.value(), size);
}

Result<std::vector<ValdiArchiveEntry>> ValdiArchive::getEntries() const {
    auto dataSection = ValdiPacket::read(_dataBegin, _dataEnd - _dataBegin);
    if (!dataSection) {
        return onModuleFailure(dataSection.moveError());
    }

    std::vector<ValdiArchiveEntry> entries;
    auto parser = Parser(dataSection.value().begin(), dataSection.value().end());
    while (!parser.isAtEnd()) {
        auto& entry = entries.emplace_back(StringBox(), nullptr, 0);
        auto filenameLength = parser.parseValue<LengthField>();
        if (!filenameLength) {
            return onModuleFailure(filenameLength.moveError());
        }

        auto filename = parseField(parser, filenameLength.value());
        if (!filename) {
            return onModuleFailure(filename.moveError());
        }

        auto dataLength = parser.parseValue<LengthField>();
        if (!dataLength) {
            return onModuleFailure(dataLength.moveError());
        }

        auto data = parseField(parser, dataLength.value());
        if (!data) {
            return onModuleFailure(data.moveError());
        }

        auto path = StringCache::getGlobal().makeString(reinterpret_cast<const char*>(filename.value().data()),
                                                        filename.value().size());

        entry.data = data.value().data();
        entry.dataLength = dataLength.value().size();
        entry.filePath = std::move(path);
    }

    return entries;
}

ValdiArchiveEntry::ValdiArchiveEntry(StringBox filePath, const Byte* data, size_t dataLength)
    : filePath(std::move(filePath)), data(data), dataLength(dataLength) {}

ValdiArchiveEntry::ValdiArchiveEntry(StringBox filePath, const StringBox& stringData)
    : ValdiArchiveEntry(std::move(filePath), reinterpret_cast<const Byte*>(stringData.getCStr()), stringData.length()) {
}

StringBox ValdiArchiveEntry::getStringData() const {
    return StringCache::getGlobal().makeString(reinterpret_cast<const char*>(data), dataLength);
}

ValdiArchiveBuilder::ValdiArchiveBuilder() = default;

void ValdiArchiveBuilder::addEntry(const ValdiArchiveEntry& entry) {
    _moduleData.reserve(_moduleData.size() + entry.dataLength + entry.filePath.length() + sizeof(uint32_t) * 2);

    auto filenameLength = static_cast<uint32_t>(entry.filePath.length());
    auto dataLength = static_cast<uint32_t>(entry.dataLength);

    auto filenamePaddingLength = computePadding(entry.filePath.length());
    auto dataPaddingLength = computePadding(entry.dataLength);
    if (filenamePaddingLength > 0) {
        filenameLength |= kPaddingBit;
    }
    if (dataPaddingLength > 0) {
        dataLength |= kPaddingBit;
    }
    _moduleData.append(reinterpret_cast<Byte*>(&filenameLength), reinterpret_cast<Byte*>(&filenameLength + 1));
    _moduleData.append(entry.filePath.getCStr(), entry.filePath.getCStr() + entry.filePath.length());
    appendPadding(filenamePaddingLength);

    _moduleData.append(reinterpret_cast<Byte*>(&dataLength), reinterpret_cast<Byte*>(&dataLength + 1));
    _moduleData.append(entry.data, entry.data + entry.dataLength);
    appendPadding(dataPaddingLength);
}

void ValdiArchiveBuilder::appendPadding(size_t padding) {
    for (size_t i = 0; i < padding; i++) {
        _moduleData.append(static_cast<Byte>(0));
    }
}

Ref<ByteBuffer> ValdiArchiveBuilder::build() const {
    auto out = makeShared<ByteBuffer>();
    ValdiPacket::write(_moduleData.data(), _moduleData.size(), *out);
    return out;
}

} // namespace Valdi
