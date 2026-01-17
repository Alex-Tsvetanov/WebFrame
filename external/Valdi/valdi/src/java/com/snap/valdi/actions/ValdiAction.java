package com.snap.valdi.actions;

import androidx.annotation.Keep;

/**
 * An Action is an abstracted callback that can be implemented by Java/Kotlin (using
 * ValdiNativeAction and ValdiRunnableAction), C++ (using ValdiCPPAction) and
 * JavaScript (ValdiJavaScriptAction).
 */
@Keep
public interface ValdiAction {

    Object perform(Object[] parameters);

}
