package com.snapchat.client.valdi;

import androidx.annotation.Keep;

@Keep
public class UndefinedValue {
    public static final UndefinedValue UNDEFINED = new UndefinedValue();
    public static final Object UNIT = kotlin.Unit.INSTANCE;

    private UndefinedValue() {}
}
