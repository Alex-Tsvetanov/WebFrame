//
//  Thread.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 8/11/19.
//

#include "valdi_core/cpp/Threading/Thread.hpp"

#include "utils/platform/JvmUtils.hpp"

#include "utils/debugging/Assert.hpp"
#include "utils/platform/TargetPlatform.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"

#if __APPLE__
#include "TargetConditionals.h"
#include "valdi_core/cpp/Threading/GCDDispatchQueue.hpp"
#else
#include <sys/resource.h>
#endif

namespace Valdi {

static PlatformThreadFactory* kFactory = nullptr;
void PlatformThreadFactory::set(PlatformThreadFactory* factory) {
    kFactory = factory;
}

PlatformThreadFactory& PlatformThreadFactory::mustGet() {
    auto* factory = kFactory;
    SC_ASSERT(factory != nullptr, "No PlatformThreadFactory set");
    return *factory;
}

#ifdef ANDROID

Thread::Thread(const StringBox& name,
               ThreadQoSClass qosClass,
               DispatchFunction runnable) // NOLINT(performance-unnecessary-value-param)
    : _runnable(std::move(runnable)) {
    _platformThread = PlatformThreadFactory::mustGet().create(name, qosClass, this);
}

Result<Ref<Thread>> Thread::create(const StringBox& name,
                                   ThreadQoSClass qosClass,
                                   DispatchFunction runnable) { // NOLINT(performance-unnecessary-value-param)
    return Valdi::makeShared<Thread>(name, qosClass, std::move(runnable));
}

void Thread::join() {
    if (_platformThread != nullptr) {
        PlatformThreadFactory::mustGet().destroy(_platformThread);
        _platformThread = nullptr;
    }
}

void Thread::setQoSClass(ThreadQoSClass qosClass) {
    if (_platformThread != nullptr) {
        PlatformThreadFactory::mustGet().setQoSClass(_platformThread, qosClass);
    }
}

#else

int qoSClassToNiceLevel(ThreadQoSClass qosClass) {
    switch (qosClass) {
        case ThreadQoSClassLowest:
            return 9;
        case ThreadQoSClassLow:
            return 5;
        case ThreadQoSClassNormal:
            return 0;
        case ThreadQoSClassHigh:
            return -2;
        case ThreadQoSClassMax:
            return -4;
    }
}

void* threadEntryPoint(void* handle) {
    Ref<Valdi::Thread> thread(reinterpret_cast<Valdi::Thread*>(handle));
    thread->handler();
    return nullptr;
}

constexpr size_t kDefaultThreadMinStackSize = 1024 * 1024;

Thread::Thread(const StringBox& name, ThreadQoSClass qosClass, DispatchFunction runnable)
    : _runnable(std::move(runnable)), _name(name), _qosClass(qosClass) {}

Thread::Thread() = default;

Result<Ref<Thread>> Thread::create(const StringBox& name, ThreadQoSClass qosClass, DispatchFunction runnable) {
    // Using pthread directly so that we can set the stack size.

    pthread_attr_t attr;
    if (pthread_attr_init(&attr) != 0) {
        return Error("Failed to get default pthread attributes");
    }

    size_t stackSize = 0;
    if (pthread_attr_getstacksize(&attr, &stackSize) == 0 && stackSize < kDefaultThreadMinStackSize) {
        auto success = pthread_attr_setstacksize(&attr, kDefaultThreadMinStackSize) == 0;
        if (!success) {
            pthread_attr_destroy(&attr);
            return Error("Failed to set default pthread stack size");
        }
    }

    auto thread = Valdi::makeShared<Thread>(name, qosClass, std::move(runnable));

    if (pthread_create(&thread->_thread, &attr, &threadEntryPoint, thread.get()) != 0) {
        pthread_attr_destroy(&attr);
        return Error("Failed to create pthread");
    }

    pthread_attr_destroy(&attr);

    return std::move(thread);
}

void Thread::join() {
    if (_thread != PTHREAD_DEFAULT_VALUE) {
        pthread_join(_thread, nullptr);
        _thread = PTHREAD_DEFAULT_VALUE;
    }
}

void Thread::setQoSClass(ThreadQoSClass qosClass) {
#if defined(SC_DESKTOP_MACOS)
    // This doesn't seem to compile on all platforms
//    pthread_set_qos_class_np(_thread, ValdiQoSClassToGCD(qosClass), 0);
#endif
}

#endif

Thread::StackAttrs Thread::getCurrentStackAttrs() {
    size_t stackSize = 0;
    void* stackAddr = nullptr;

    auto thread = pthread_self();

#if __APPLE__
    stackAddr = pthread_get_stackaddr_np(thread);
    stackSize = pthread_get_stacksize_np(thread);

    // Check whether stackAddr is the base or end of the stack.
    int stackVariable;
    if (stackAddr > &stackVariable) {
        stackAddr = reinterpret_cast<uint8_t*>(stackAddr) - stackSize;
    }
#else
    pthread_attr_t attr;
    if (pthread_getattr_np(thread, &attr) != 0) {
        SC_ASSERT_FAIL("pthread_getattr_np() failed");
    }

    if (pthread_attr_getstack(&attr, &stackAddr, &stackSize) != 0) {
        SC_ASSERT_FAIL("pthread_attr_getstack() failed");
    }

    pthread_attr_destroy(&attr);
#endif
    return Thread::StackAttrs(stackAddr, stackSize);
}

Thread::~Thread() {
    if (Thread::getCurrent() != this) {
        join();
    }
}

thread_local Thread* currentThread = nullptr;

void Thread::handler() {
    currentThread = this;

#if !defined(ANDROID)
#if __APPLE__
    pthread_set_qos_class_self_np(ValdiQoSClassToGCD(_qosClass), 0);
    pthread_setname_np(_name.getCStr());
#else
    setpriority(PRIO_PROCESS, 0, qoSClassToNiceLevel(_qosClass));
#endif
#endif

    _runnable();
}

Thread* Thread::getCurrent() {
    return currentThread;
}

void Thread::setCurrent(Thread* thread) {
    currentThread = thread;
}

} // namespace Valdi
