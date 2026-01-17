//
//  SafeContainer.hpp
//  snap_drawing
//
//  Created by Simon Corsin on 10/24/22.
//

#pragma once

#include "utils/base/NonCopyable.hpp"
#include <cstddef>

namespace snap::drawing {

class SafeContainerBase {
public:
    class Write : public snap::NonCopyable {
    public:
        explicit Write(SafeContainerBase* wrapper);
        ~Write();

    private:
        SafeContainerBase* _wrapper;
    };

    class Read : public snap::NonCopyable {
    public:
        explicit Read(SafeContainerBase* wrapper);
        ~Read();

    private:
        SafeContainerBase* _wrapper;
    };

private:
    int _readCount = 0;
    int _writeCount = 0;

    friend Write;
    friend Read;

    void onReadBegin();
    void onReadEnd();

    void onWriteBegin();
    void onWriteEnd();
};

/**
 SafeContainer is a general purpose wrapper around a container that exposes
 read and write "locks" semantics. It helps ensuring that mutations to the underlying
 container are not happening while a read is in progress and vice versa.
 */
template<typename T>
class SafeContainer : public SafeContainerBase {
public:
    class WriteAccess : public SafeContainerBase::Write {
    public:
        WriteAccess(SafeContainerBase* wrapper, T& container)
            : SafeContainerBase::Write(wrapper), _container(container) {}
        ~WriteAccess() = default;

        constexpr T* operator->() const {
            return &_container;
        }

        constexpr T& operator*() const {
            return _container;
        }

        T& get() const {
            return _container;
        }

        auto begin() const {
            return _container.begin();
        }

        auto end() const {
            return _container.end();
        }

    private:
        T& _container;
    };

    class ReadAccess : public SafeContainerBase::Read {
    public:
        ReadAccess(SafeContainerBase* wrapper, const T& container)
            : SafeContainerBase::Read(wrapper), _container(container) {}
        ~ReadAccess() = default;

        constexpr const T* operator->() const {
            return &_container;
        }

        constexpr const T& operator*() const {
            return _container;
        }

        const T& get() const {
            return _container;
        }

        auto begin() const {
            return _container.begin();
        }

        auto end() const {
            return _container.end();
        }

    private:
        const T& _container;
    };

    WriteAccess writeAccess() {
        return WriteAccess(this, _container);
    }

    ReadAccess readAccess() const {
        // Casting to mutable to preserve read-only semantics on the caller, even
        // if we have to increment/decrement an integer under the hood
        return ReadAccess(const_cast<SafeContainer<T>*>(this), _container);
    }

    size_t size() const {
        return _container.size();
    }

    bool empty() const {
        return _container.empty();
    }

private:
    T _container;
};
} // namespace snap::drawing
