#pragma once

#include "../cpp/DataRef.hpp"
#include "../cpp/DataView.hpp"
#include "djinni_support.hpp"

namespace djinni::swift {

class DataViewAdaptor {
public:
    using CppType = DataView;
    static CppType toCpp(const AnyValue& s);
    static AnyValue fromCpp(CppType c);
};

class DataRefAdaptor {
public:
    using CppType = DataRef;
    static CppType toCpp(const AnyValue& s);
    static AnyValue fromCpp(const CppType& c);
};

} // namespace djinni::swift
