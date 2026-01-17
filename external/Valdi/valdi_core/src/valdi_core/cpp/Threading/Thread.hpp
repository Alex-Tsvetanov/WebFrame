//
//  Thread.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 8/11/19.
//

#pragma once

#include "utils/base/NonCopyable.hpp"

#include "valdi_core/cpp/Threading/ThreadQoSClass.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"

#include "valdi_core/cpp/Utils/Function.hpp"
#include <pthread.h>

#if __APPLE__
#define PTHREAD_DEFAULT_VALUE nullptr
#else
#define PTHREAD_DEFAULT_VALUE 0
#endif

namespace Valdi {

class Thread;
class PlatformThread;

class PlatformThreadFactory {
public:
    virtual ~PlatformThreadFactory() = default;

    static void set(PlatformThreadFactory* factory);
    static PlatformThreadFactory& mustGet();

    virtual PlatformThread* create(const Valdi::StringBox& name,
                                   Valdi::ThreadQoSClass qosClass,
                                   Valdi::Thread* handler) = 0;
    virtual void destroy(PlatformThread* thread) = 0;
    virtual void setQoSClass(PlatformThread* thread, Valdi::ThreadQoSClass qosClass) = 0;
};

class Thread : public SimpleRefCountable, public snap::NonCopyable {
public:
    struct StackAttrs {
        void* minFrameAddress;
        size_t stackSize;

        constexpr StackAttrs(void* minFrameAddress, size_t stackSize)
            : minFrameAddress(minFrameAddress), stackSize(stackSize) {}
    };

    // DO NOT CALL THIS CONSTRUCTOR DIRECTLY, call create() instead.
    // This is public to allow the create function to use make_shared.
    Thread(const StringBox& name, ThreadQoSClass qosClass, DispatchFunction runnable);
    Thread();
    ~Thread() override;

    void join();

    void setQoSClass(ThreadQoSClass qosClass);

    // Only made public for internal access.
    // DO NOT CALL THIS METHOD DIRECTLY.
    void handler();

    static StackAttrs getCurrentStackAttrs();
    static Thread* getCurrent();
    static void setCurrent(Thread* thread);

    static Result<Ref<Thread>> create(const StringBox& name, ThreadQoSClass qosClass, DispatchFunction runnable);

private:
    DispatchFunction _runnable;

#ifdef ANDROID
    PlatformThread* _platformThread = nullptr;
#else
    StringBox _name;
    ThreadQoSClass _qosClass;
    pthread_t _thread = PTHREAD_DEFAULT_VALUE;
#endif
};
} // namespace Valdi

#define MAIN_THREAD_INIT()                                                                                             \
    Valdi::Thread __mainThread;                                                                                        \
    Valdi::Thread::setCurrent(&__mainThread);
