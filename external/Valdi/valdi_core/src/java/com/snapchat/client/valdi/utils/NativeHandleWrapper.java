package com.snapchat.client.valdi.utils;

import androidx.annotation.Keep;

@Keep
public abstract class NativeHandleWrapper {

    private long mNativeHandle;

    public NativeHandleWrapper(long nativeHandle) {
        mNativeHandle = nativeHandle;
    }

    public long getNativeHandle() {
        return mNativeHandle;
    }

    protected abstract void destroyHandle(long handle);

    private long removeNativeHandle() {
        long handle = this.mNativeHandle;
        this.mNativeHandle = 0;
        return handle;
    }

    public void setNativeHandle(long handle) {
        this.mNativeHandle = handle;
    }

    public void destroy() {
        long handle;
        synchronized(this) {
            handle = this.removeNativeHandle();
        }
        if (handle != 0L) {
            destroyHandle(handle);
        }
    }

    protected void finalize() throws java.lang.Throwable {
        // Avoid the synchronized in finalize(), as we know destroy()
        // couldn't have been called at the same time.
        long handle = this.removeNativeHandle();
        if (handle != 0L) {
            this.destroyHandle(handle);
        }
        super.finalize();
    }

}