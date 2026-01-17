# Client wraper libs
-keep public enum com.snapchat.client.** {
    **[] $VALUES;
    public *;
}

-keep class com.snapchat.djinni.** { *; }

# For serializing Djinni protos
-keep class com.google.protobuf.nano.MessageNano { *; }