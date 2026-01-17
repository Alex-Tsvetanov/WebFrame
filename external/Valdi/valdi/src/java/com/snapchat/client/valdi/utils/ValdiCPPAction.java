package com.snapchat.client.valdi.utils;

import com.snap.valdi.actions.ValdiAction;
import androidx.annotation.Keep;

// Kept here for compat, as some consumers are referencing to this class directly
@Keep
public abstract class ValdiCPPAction extends CppObjectWrapper implements ValdiAction {

    public ValdiCPPAction(long nativeHandle) {
        super(nativeHandle);
    }

}
