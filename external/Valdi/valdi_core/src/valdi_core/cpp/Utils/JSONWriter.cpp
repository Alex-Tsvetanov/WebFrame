//
//  JSONWriter.cpp
//  ValdiRuntime
//
//  Created by Simon Corsin on 12/21/23.
//  Copyright © 2023 Snap Inc. All rights reserved.
//

#include "valdi_core/cpp/Utils/JSONWriter.hpp"
#include "valdi_core/cpp/Utils/StaticString.hpp"

#include "json/writer.h"

namespace Valdi {

static bool needsUnicodeEscaping(std::string_view str) {
    const auto* it = reinterpret_cast<const Byte*>(str.data());
    const auto* end = it + str.size();

    while (it != end) {
        auto c = *it;
        if (c >= 0x7f) {
            return true;
        }
        it++;
    }

    return false;
}

JSONWriter::JSONWriter(ByteBuffer& output) : _output(output) {}

void JSONWriter::writeNull() {
    write("null");
}

void JSONWriter::writeString(std::string_view str) {
    if (needsUnicodeEscaping(str)) {
        writeUnicodeString(str);
    } else {
        writeASCIIString(str);
    }
}

void JSONWriter::writeInt(int64_t i) {
    write(std::to_string(i));
}

void JSONWriter::writeInt(int32_t i) {
    write(std::to_string(i));
}

void JSONWriter::writeDouble(double d) {
    write(std::to_string(d));
}

void JSONWriter::writeBool(bool b) {
    if (b) {
        write("true");
    } else {
        write("false");
    }
}

void JSONWriter::writeProperty(std::string_view str) {
    writeString(str);
    writeColon();
}

void JSONWriter::writeColon() {
    _output.append(':');
}

void JSONWriter::writeBeginObject() {
    _output.append('{');
}

void JSONWriter::writeEndObject() {
    _output.append('}');
}

void JSONWriter::writeBeginArray() {
    _output.append('[');
}

void JSONWriter::writeEndArray() {
    _output.append(']');
}

void JSONWriter::writeComma() {
    _output.append(',');
}

void JSONWriter::writeNewLine() {
    _output.append('\n');
}

void JSONWriter::write(std::string_view str) {
    _output.append(str.begin(), str.end());
}

void JSONWriter::writeStringArray(const char* const* begin, const char* const* end) {
    writeArray(begin, end, [&](const auto* str) { writeString(std::string_view(str)); });
}

void JSONWriter::writeCommaDelimitedStrings(const char* const* begin, const char* const* end) {
    writeCommaDelimited(begin, end, [&](const auto& str) { writeString(std::string_view(str)); });
}

void JSONWriter::writeASCIIString(std::string_view str) {
    _output.append('"');
    const auto* it = reinterpret_cast<const Byte*>(str.data());
    const auto* end = it + str.size();

    while (it != end) {
        auto c = *it;
        switch (c) {
            case '\t':
                write("\\t");
                break;
            case '\n':
                write("\\n");
                break;
            case '\v':
                write("\\v");
                break;
            case '\f':
                write("\\f");
                break;
            case '\r':
                write("\\r");
                break;
            case '\x1b':
                // Hex isn't valid JSON even when escaped
                // so we must unicde instead.
                writeSmallHex(c);
                break;
            case '"':
            case '\\':
                _output.append(static_cast<Byte>('\\'));
                _output.append(c);
                break;
            default:
                if (c < 0x20) {
                    writeSmallHex(c);
                } else {
                    _output.append(c);
                }
        }
        it++;
    }
    _output.append('"');
}

void JSONWriter::writeUnicodeString(std::string_view str) {
    Json::FastWriter writer;
    writer.omitEndingLineFeed();
    auto result = writer.write(Json::Value(str.data(), str.data() + str.size()));
    write(result);
}

void JSONWriter::writeSmallHex(Byte c) {
    static const char* kDigits = "0123456789ABCDEF";

    write("\\u00");
    _output.append(static_cast<Byte>(kDigits[(c >> 4) & 0xf]));
    _output.append(static_cast<Byte>(kDigits[c & 0xf]));
}

} // namespace Valdi
