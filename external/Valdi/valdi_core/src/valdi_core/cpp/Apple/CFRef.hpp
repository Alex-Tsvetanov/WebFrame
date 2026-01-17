#pragma once

#include <CoreFoundation/CoreFoundation.h>

namespace Valdi {

/**
 * Wrapper around a CoreFoundation reference. Takes care of
 * releasing/retaining the reference using CFRelease and CFRetain
 * automatically.
 */
template<typename T>
class CFRef {
public:
    CFRef() = default;
    CFRef(T&& ptr) : _ptr(ptr) {}
    CFRef(const T& ptr) : _ptr(ptr) {
        retain();
    }

    CFRef(CFRef<T>&& ref) : _ptr(ref._ptr) {
        ref._ptr = nullptr;
    }

    CFRef(const CFRef<T>& ref) : _ptr(ref._ptr) {
        retain();
    }

    ~CFRef() {
        release();
    }

    T get() const {
        return _ptr;
    }

    T* unsafeGetAddress() {
        return &_ptr;
    }

    T extractRef() {
        auto ref = _ptr;
        _ptr = nullptr;
        return ref;
    }

    CFRef<T>& operator=(T ptr) {
        release();
        _ptr = ptr;
        retain();
        return *this;
    }

    CFRef<T>& operator=(const CFRef<T>& ref) {
        release();
        _ptr = ref._ptr;
        retain();
        return *this;
    }

    CFRef<T>& operator=(CFRef<T>&& ref) {
        release();
        _ptr = ref._ptr;
        ref._ptr = nullptr;
        return *this;
    }

    long retainCount() const {
        return _ptr != nullptr ? CFGetRetainCount(_ptr) : 0;
    }

private:
    T _ptr = nullptr;

    void release() {
        if (_ptr != nullptr) {
            CFRelease(_ptr);
            _ptr = nullptr;
        }
    }

    void retain() {
        if (_ptr != nullptr) {
            CFRetain(_ptr);
        }
    }
};

template<typename T>
inline bool operator==(const CFRef<T>& left, const std::nullptr_t& right) {
    return left.get() == right;
}

template<typename T>
inline bool operator==(const std::nullptr_t& left, const CFRef<T>& right) {
    return left == right.get();
}

template<typename T>
inline bool operator!=(const CFRef<T>& left, const std::nullptr_t& right) {
    return !(left == right);
}

template<typename T>
inline bool operator!=(const std::nullptr_t& left, const CFRef<T>& right) {
    return !(left == right);
}

} // namespace Valdi
