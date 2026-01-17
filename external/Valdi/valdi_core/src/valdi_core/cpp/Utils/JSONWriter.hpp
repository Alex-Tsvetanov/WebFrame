//
//  JSONWriter.hpp
//  ValdiRuntime
//
//  Created by Simon Corsin on 12/21/23.
//  Copyright © 2023 Snap Inc. All rights reserved.
//

#pragma once

#include "valdi_core/cpp/Utils/ByteBuffer.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"

namespace Valdi {

class ValueTypedObject;

class JSONWriter {
public:
    JSONWriter(ByteBuffer& output);

    void writeNull();

    void writeString(std::string_view str);

    void writeInt(int64_t i);
    void writeInt(int32_t i);
    void writeDouble(double d);
    void writeBool(bool b);

    void writeProperty(std::string_view str);

    void writeColon();

    void writeBeginObject();
    void writeEndObject();

    void writeBeginArray();
    void writeEndArray();

    void writeComma();

    void writeNewLine();

    template<typename Iterator, typename F>
    void writeCommaDelimited(Iterator begin, Iterator end, F&& fn) {
        auto first = true;
        auto it = begin;
        while (it != end) {
            if (first) {
                first = false;
            } else {
                writeComma();
            }
            fn(*it);

            it++;
        }
    }

    template<typename Iterator, typename F>
    void writeArray(Iterator begin, Iterator end, F&& fn) {
        writeBeginArray();
        writeCommaDelimited(begin, end, std::move(fn));
        writeEndArray();
    }

    template<typename C, typename F>
    void writeArray(const C& container, F&& fn) {
        writeArray(container.begin(), container.end(), std::move(fn));
    }

    void writeStringArray(std::initializer_list<const char*> stringArray) {
        writeStringArray(stringArray.begin(), stringArray.end());
    }

    void writeStringArray(const char* const* begin, const char* const* end);

    void writeCommaDelimitedStrings(std::initializer_list<const char*> stringArray) {
        writeCommaDelimitedStrings(stringArray.begin(), stringArray.end());
    }

    void writeCommaDelimitedStrings(const char* const* begin, const char* const* end);

private:
    ByteBuffer& _output;

    void write(std::string_view str);
    void writeUnicodeString(std::string_view str);
    void writeASCIIString(std::string_view str);

    void writeSmallHex(Byte c);
};

} // namespace Valdi
