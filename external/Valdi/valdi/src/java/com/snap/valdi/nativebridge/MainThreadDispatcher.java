package com.snap.valdi.nativebridge;

import android.annotation.SuppressLint;
import android.os.Looper;
import android.view.Choreographer;

import com.snapchat.client.valdi.NativeBridge;

import androidx.annotation.Keep;

import com.snap.valdi.exceptions.ValdiException;
import com.snap.valdi.logger.LogLevel;
import com.snap.valdi.logger.Logger;
import com.snap.valdi.utils.MainThreadUtils;
import kotlin.Unit;
import kotlin.jvm.functions.Function0;

/**
 * This class is used by Valdi C++ to forward calls to the UI thread.
 */
public class MainThreadDispatcher {

    private Logger logger;

    public MainThreadDispatcher(Logger logger) {
        this.logger = logger;
    }

    @SuppressWarnings("unused")
    @Keep
    void runOnMainThread(final long handle) {
        MainThreadUtils.dispatchOnMainThread(new Function0<Unit>() {
            @Override
            public Unit invoke() {
                try {
                    NativeBridge.performCallback(handle);
                } catch (ValdiException e) {
                    logger.log(LogLevel.Companion.getERROR(), e.getMessage());
                }
                return Unit.INSTANCE;
            }
        });
    }

    @SuppressWarnings("unused")
    @Keep
    void runOnMainThreadDelayed(final long delayMs, final long handle) {
        MainThreadUtils.runOnMainThreadDelayed(delayMs, new Function0<Unit>() {
            @Override
            public Unit invoke() {
                try {
                    NativeBridge.performCallback(handle);
                } catch (ValdiException e) {
                    logger.log(LogLevel.Companion.getERROR(), e.getMessage());
                }
                return Unit.INSTANCE;
            }
        });
    }

}
