//
//  ByteBuffer.hpp
//  valdi-desktop-apple
//
//  Created by Simon Corsin on 9/17/20.
//

#pragma once

#include "valdi_core/cpp/Utils/Bytes.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"

namespace Valdi {

/**
 A simple byte buffer, designed as a lightweight replacement to std::vector<Byte>.
 */
class ByteBuffer : public SimpleRefCountable {
public:
    ByteBuffer();
    ByteBuffer(ByteBuffer&& other) noexcept;
    ByteBuffer(const ByteBuffer& other);
    explicit ByteBuffer(const std::string& stringBufferToCopy);
    explicit ByteBuffer(std::string_view stringBufferToCopy);
    explicit ByteBuffer(const char* nullTerminatedString);
    ByteBuffer(const Byte* begin, const Byte* end);

    ~ByteBuffer() override;

    ByteBuffer& operator=(const ByteBuffer& other);
    ByteBuffer& operator=(ByteBuffer&& other) noexcept;
    bool operator==(const ByteBuffer& other) const;
    bool operator!=(const ByteBuffer& other) const;

    void clear();

    constexpr Byte* data() const {
        return _data;
    }

    constexpr size_t size() const {
        return _size;
    }

    constexpr bool empty() const {
        return _size == 0;
    }

    constexpr size_t capacity() const {
        return _capacity;
    }

    constexpr Byte* begin() const {
        return data();
    }

    constexpr Byte* end() const {
        return begin() + size();
    }

    void reserve(size_t capacity);

    constexpr Byte* operator[](size_t i) const {
        return _data + i;
    }

    /**
     Append N bytes to the buffer, and return a writable pointer to the beginning
     of the added region
     */
    Byte* appendWritable(size_t size);

    /**
     Append and write the given region to the buffer
     */
    void append(const Byte* begin, const Byte* end);

    /**
     Append and write the given region to the buffer
     */
    void append(const char* begin, const char* end);

    /**
     Append and write the given array to the buffer
     */
    void append(const Bytes& region);

    /**
     Append and write the given string array to the buffer
     */
    void append(const std::string_view& stringRegion);

    /**
     Append a single char to the buffer
     */
    void append(char c);

    /**
     Append a single byte to the buffer
     */
    void append(Byte c);

    /**
     Set the entire ByteBuffer to the given region
     */
    void set(const Byte* begin, const Byte* end);

    /**
     Set the entire ByteBuffer to the given region
     */
    void set(std::initializer_list<Byte> bytes) {
        set(bytes.begin(), bytes.end());
    }

    /**
     Remove the first N bytes from the buffer
     */
    void shift(size_t size);

    /**
     Resize the entire buffer to be the given size.
     Capacity will be reduced if needed.
     */
    void resize(size_t size);

    /**
     Shrink the ByteBuffer so that it fits the capacity.
     */
    void shrinkToFit();

    /**
     Return a BytesView representation of this ByteBuffer.
     This ByteBuffer MUST be allocated through makeShared<>
     */
    BytesView toBytesView();

    std::string_view toStringView() const;

    /**
     Return a copy of this ByteBuffer as bytes
     */
    std::vector<uint8_t> toBytesVec() const;

private:
    size_t _size = 0;
    size_t _capacity = 0;
    Byte* _data = nullptr;

    void reallocate(size_t capacity);
};

} // namespace Valdi
