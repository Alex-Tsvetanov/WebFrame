#if defined(__APPLE__)

#include "valdi_core/cpp/Apple/CFConversions.hpp"
#include "valdi_core/cpp/Utils/ByteBuffer.hpp"
#include "valdi_core/cpp/Utils/Bytes.hpp"
#include "valdi_core/cpp/Utils/SmallVector.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"

namespace Valdi {

class CFRefVector {
public:
    CFRefVector() = default;
    ~CFRefVector() = default;

    void append(CFRef<CFTypeRef> value) {
        _vec.emplace_back(std::move(value));
    }

    void reserve(size_t size) {
        _vec.reserve(size);
    }

    size_t size() const {
        return _vec.size();
    }

    const void** data() {
        // For this to work without an intermediate, this requires CFRef to be exactly
        // one pointer size.
        static_assert(sizeof(CFRef<CFTypeRef>) == sizeof(void*));
        return _vec.data()->unsafeGetAddress();
    }

private:
    SmallVector<CFRef<CFTypeRef>, 16> _vec;
};

static CFRef<CFTypeRef> makeCFNumber(CFNumberType type, const void* value) {
    return CFRef<CFTypeRef>(CFNumberCreate(kCFAllocatorDefault, type, value));
}

CFRef<CFTypeRef> toCFNumber(int32_t value) {
    return makeCFNumber(kCFNumberSInt32Type, &value);
}

CFRef<CFTypeRef> toCFNumber(int64_t value) {
    return makeCFNumber(kCFNumberSInt64Type, &value);
}

CFRef<CFTypeRef> toCFNumber(bool value) {
    int8_t resolvedValue = value ? 1 : 0;
    return makeCFNumber(kCFNumberSInt8Type, &resolvedValue);
}

CFRef<CFTypeRef> toCFNumber(float value) {
    return makeCFNumber(kCFNumberFloat32Type, &value);
}

CFRef<CFTypeRef> toCFNumber(double value) {
    return makeCFNumber(kCFNumberFloat64Type, &value);
}

template<typename T>
CFRefVector toCFRefVector(const T* values, size_t size) {
    CFRefVector output;
    output.reserve(size);

    for (size_t i = 0; i < size; i++) {
        output.append(toCFValue(values[i]));
    }

    return output;
}

CFRef<CFTypeRef> toCFArray(std::initializer_list<float> values) {
    auto valuesAsNumbers = toCFRefVector(values.begin(), values.size());

    return CFRef<CFTypeRef>(
        CFArrayCreate(kCFAllocatorDefault, valuesAsNumbers.data(), valuesAsNumbers.size(), &kCFTypeArrayCallBacks));
}

CFRef<CFTypeRef> toCFValue(int32_t value) {
    return toCFNumber(value);
}

CFRef<CFTypeRef> toCFValue(int64_t value) {
    return toCFNumber(value);
}

CFRef<CFTypeRef> toCFValue(bool value) {
    return toCFNumber(value);
}

CFRef<CFTypeRef> toCFValue(float value) {
    return toCFNumber(value);
}

CFRef<CFTypeRef> toCFValue(double value) {
    return toCFNumber(value);
}

static CFStringRef toCFStringInner(std::string_view value) {
    return CFStringCreateWithBytes(
        kCFAllocatorDefault, reinterpret_cast<const UInt8*>(value.data()), value.size(), kCFStringEncodingUTF8, false);
}

static CFStringRef toCFStringInner(const char* value) {
    return CFStringCreateWithCString(kCFAllocatorDefault, value, kCFStringEncodingUTF8);
}

CFRef<CFTypeRef> toCFValue(std::string_view value) {
    return CFRef<CFTypeRef>(toCFStringInner(value));
}

CFRef<CFTypeRef> toCFValue(const char* value) {
    return CFRef<CFTypeRef>(toCFStringInner(value));
}

CFRef<CFTypeRef> toCFValue(const Byte* data, size_t length) {
    return CFRef<CFTypeRef>(CFDataCreate(kCFAllocatorDefault, data, length));
}

CFRef<CFTypeRef> toCFValue(const BytesView& value) {
    return toCFValue(value.data(), value.size());
}

CFRef<CFTypeRef> toCFValue(const ByteBuffer& value) {
    return toCFValue(value.data(), value.size());
}

CFRef<CFTypeRef> toCFValue(CFTypeRef cfType) {
    return CFRef<CFTypeRef>(cfType);
}

CFRef<CFStringRef> toCFString(std::string_view value) {
    return CFRef<CFStringRef>(toCFStringInner(value));
}

CFRef<CFStringRef> toCFString(const char* value) {
    return CFRef<CFStringRef>(toCFStringInner(value));
}

std::string toSTDString(CFStringRef cfString) {
    if (const char* direct = CFStringGetCStringPtr(cfString, kCFStringEncodingUTF8)) {
        return std::string(direct);
    }

    auto len = CFStringGetLength(cfString);
    auto maxBytes = CFStringGetMaximumSizeForEncoding(len, kCFStringEncodingUTF8) + 1;

    std::string buffer(static_cast<std::size_t>(maxBytes), '\0');

    if (CFStringGetCString(cfString, buffer.data(), buffer.size(), kCFStringEncodingUTF8)) {
        buffer.resize(std::strlen(buffer.data()));
        return buffer;
    }

    return "";
}

StringBox toStringBox(CFStringRef cfString) {
    return StringCache::getGlobal().makeString(toSTDString(cfString));
}

std::string getCFValueDescription(CFTypeRef cfType) {
    CFRef<CFStringRef> description = CFCopyDescription(cfType);
    return toSTDString(description.get());
}

} // namespace Valdi

#endif