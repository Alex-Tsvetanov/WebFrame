//
//  SafeContainer.cpp
//  snap_drawing
//
//  Created by Simon Corsin on 10/24/22.
//

#include "snap_drawing/cpp/Utils/SafeContainer.hpp"
#include "utils/debugging/Assert.hpp"

namespace snap::drawing {

SafeContainerBase::Write::Write(SafeContainerBase* wrapper) : _wrapper(wrapper) {
    wrapper->onWriteBegin();
}

SafeContainerBase::Write::~Write() {
    _wrapper->onWriteEnd();
}

SafeContainerBase::Read::Read(SafeContainerBase* wrapper) : _wrapper(wrapper) {
    wrapper->onReadBegin();
}

SafeContainerBase::Read::~Read() {
    _wrapper->onReadEnd();
}

void SafeContainerBase::onReadBegin() {
    if (_writeCount > 0) {
        SC_ASSERT_FAIL("Cannot acquire read lock: write lock already acquired");
    }
    _readCount++;
}

void SafeContainerBase::onReadEnd() {
    _readCount--;
}

void SafeContainerBase::onWriteBegin() {
    if (_readCount > 0) {
        SC_ASSERT_FAIL("Cannot acquire write lock: read lock already acquired");
    }
    if (_writeCount > 0) {
        SC_ASSERT_FAIL("Cannot acquire write lock: write lock already acquired");
    }
    _writeCount++;
}

void SafeContainerBase::onWriteEnd() {
    _writeCount--;
}

} // namespace snap::drawing
