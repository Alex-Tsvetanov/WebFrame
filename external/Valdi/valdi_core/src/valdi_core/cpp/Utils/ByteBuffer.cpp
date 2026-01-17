//
//  ByteBuffer.cpp
//  valdi-desktop-apple
//
//  Created by Simon Corsin on 9/17/20.
//

#include "valdi_core/cpp/Utils/ByteBuffer.hpp"

namespace Valdi {

constexpr size_t kMinAllocationSize = sizeof(size_t);

static inline void deallocateBuffer(Byte* buffer) {
    free(buffer); // NOLINT(cppcoreguidelines-no-malloc)
}

static inline Byte* allocateBuffer(size_t size) {
    if (size == 0) {
        return nullptr;
    }

    return reinterpret_cast<Byte*>(malloc(sizeof(Byte) * size)); // NOLINT(cppcoreguidelines-no-malloc)
}

ByteBuffer::ByteBuffer() = default;

ByteBuffer::ByteBuffer(ByteBuffer&& other) noexcept
    : _size(other._size), _capacity(other._capacity), _data(other._data) {
    other._size = 0;
    other._capacity = 0;
    other._data = nullptr;
}

ByteBuffer::ByteBuffer(const ByteBuffer& other) {
    if (other.size() != 0) {
        reallocate(other.size());
        std::memcpy(_data, other.data(), other.size());
        _size = other.size();
    }
}

ByteBuffer::ByteBuffer(const std::string& stringBufferToCopy) : ByteBuffer(std::string_view(stringBufferToCopy)) {}
ByteBuffer::ByteBuffer(const char* nullTerminatedString) : ByteBuffer(std::string_view(nullTerminatedString)) {}

ByteBuffer::ByteBuffer(std::string_view stringBufferToCopy) : ByteBuffer() {
    set(reinterpret_cast<const Byte*>(stringBufferToCopy.data()),
        reinterpret_cast<const Byte*>(stringBufferToCopy.data() + stringBufferToCopy.size()));
}

ByteBuffer::ByteBuffer(const Byte* begin, const Byte* end) : ByteBuffer() {
    set(begin, end);
}

ByteBuffer::~ByteBuffer() {
    deallocateBuffer(_data);
}

ByteBuffer& ByteBuffer::operator=(ByteBuffer&& other) noexcept {
    if (&other != this) {
        deallocateBuffer(_data);

        _size = other._size;
        _capacity = other._capacity;
        _data = other._data;

        other._size = 0;
        other._capacity = 0;
        other._data = nullptr;
    }
    return *this;
}

ByteBuffer& ByteBuffer::operator=(const ByteBuffer& other) {
    if (&other != this) {
        deallocateBuffer(_data);

        _size = 0;
        _capacity = 0;
        _data = nullptr;

        if (other.size() != 0) {
            reallocate(other.size());
            std::memcpy(_data, other.data(), other.size());
            _size = other.size();
        }
    }

    return *this;
}

bool ByteBuffer::operator==(const ByteBuffer& other) const {
    if (_size != other._size) {
        return false;
    }
    for (size_t i = 0; i < _size; i++) {
        if (_data[i] != other._data[i]) {
            return false;
        }
    }

    return true;
}

bool ByteBuffer::operator!=(const ByteBuffer& other) const {
    return !(*this == other);
}

void ByteBuffer::clear() {
    _size = 0;
}

void ByteBuffer::reserve(size_t capacity) {
    if (capacity > _capacity) {
        reallocate(capacity);
    }
}

static unsigned long nextPowerOfTwo(unsigned long v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

Byte* ByteBuffer::appendWritable(size_t size) {
    if (size == 0) {
        return _data + _size;
    }

    auto nextSize = _size + size;
    if (nextSize > _capacity) {
        reallocate(
            std::max(static_cast<size_t>(nextPowerOfTwo(static_cast<unsigned long>(nextSize))), kMinAllocationSize));
    }

    auto* ptr = _data + _size;
    _size = nextSize;
    return ptr;
}

void ByteBuffer::append(const Byte* begin, const Byte* end) {
    auto size = static_cast<size_t>(end - begin);
    if (size > 0) {
        auto* bytes = appendWritable(size);
        std::memcpy(bytes, begin, size);
    }
}

void ByteBuffer::append(const char* begin, const char* end) {
    append(reinterpret_cast<const Byte*>(begin), reinterpret_cast<const Byte*>(end));
}

void ByteBuffer::append(const std::string_view& stringRegion) {
    append(stringRegion.data(), stringRegion.data() + stringRegion.size());
}

void ByteBuffer::append(char c) {
    append(static_cast<Byte>(c));
}

void ByteBuffer::append(Byte c) {
    *appendWritable(1) = c;
}

void ByteBuffer::set(const Byte* begin, const Byte* end) {
    clear();

    auto size = static_cast<size_t>(end - begin);

    if (size != capacity()) {
        reallocate(size);
    }
    append(begin, end);
}

void ByteBuffer::resize(size_t size) {
    if (size != capacity()) {
        reallocate(size);
    }
    _size = size;
}

void ByteBuffer::shift(size_t size) {
    auto newSize = this->size() - size;
    std::memmove(data(), data() + size, newSize);
    _size = newSize;
}

void ByteBuffer::reallocate(size_t capacity) {
    auto* newData = allocateBuffer(capacity);

    auto* oldData = _data;

    if (newData != nullptr && oldData != nullptr) {
        std::memcpy(newData, oldData, std::min(_capacity, capacity));
    }
    deallocateBuffer(oldData);

    _capacity = capacity;
    _data = newData;
}

void ByteBuffer::shrinkToFit() {
    if (_capacity != _size) {
        reallocate(_size);
    }
}

BytesView ByteBuffer::toBytesView() {
    return BytesView(strongSmallRef(this), data(), size());
}

std::string_view ByteBuffer::toStringView() const {
    return std::string_view(reinterpret_cast<const char*>(data()), size());
}

std::vector<uint8_t> ByteBuffer::toBytesVec() const {
    std::vector<uint8_t> out;
    out.insert(out.end(), begin(), end());
    return out;
}

} // namespace Valdi
