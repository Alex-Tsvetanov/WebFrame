#pragma once

#include "valdi_core/cpp/Apple/CFRef.hpp"
#include "valdi_core/cpp/Utils/Byte.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"
#include <CoreFoundation/CoreFoundation.h>
#include <string>

namespace Valdi {

class BytesView;
class ByteBuffer;

CFRef<CFTypeRef> toCFNumber(int32_t value);
CFRef<CFTypeRef> toCFNumber(int64_t value);
CFRef<CFTypeRef> toCFNumber(bool value);
CFRef<CFTypeRef> toCFNumber(float value);
CFRef<CFTypeRef> toCFNumber(double value);

CFRef<CFTypeRef> toCFArray(std::initializer_list<float> values);

CFRef<CFTypeRef> toCFValue(int32_t value);

CFRef<CFTypeRef> toCFValue(int64_t value);

CFRef<CFTypeRef> toCFValue(bool value);

CFRef<CFTypeRef> toCFValue(float value);

CFRef<CFTypeRef> toCFValue(double value);

CFRef<CFTypeRef> toCFValue(std::string_view value);

CFRef<CFTypeRef> toCFValue(const char* value);

CFRef<CFTypeRef> toCFValue(const Byte* data, size_t length);

CFRef<CFTypeRef> toCFValue(const BytesView& value);

CFRef<CFTypeRef> toCFValue(const ByteBuffer& value);

CFRef<CFTypeRef> toCFValue(CFTypeRef cfType);

CFRef<CFStringRef> toCFString(std::string_view value);

CFRef<CFStringRef> toCFString(const char* value);

template<typename T>
CFRef<CFTypeRef> toCFValue(const CFRef<T>& cfRef) {
    return toCFValue(cfRef.get());
}

std::string toSTDString(CFStringRef cfString);
StringBox toStringBox(CFStringRef cfString);

std::string getCFValueDescription(CFTypeRef cfType);

} // namespace Valdi
