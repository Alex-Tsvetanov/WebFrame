#### Valdi classes

-keep public class * extends com.snap.valdi.utils.AutoDisposable {
    public protected *;
}

-keep public class * extends com.snap.valdi.views.ValdiView {
    public protected *;
}

-keep public class * implements com.snap.valdi.views.ValdiRecyclableView {
    public protected *;
}

#### Valdi generated classes

# Keep Kotlin Function interfaces, as they are used through reflections
-keepclassmembers,includedescriptorclasses,allowobfuscation interface kotlin.jvm.functions.* { *; }

# Keep the Valdi runtime annotations
-keep,allowobfuscation @interface com.snap.valdi.schema.ValdiClass { *; }
-keep,allowobfuscation @interface com.snap.valdi.schema.ValdiInterface { *; }
-keep,allowobfuscation @interface com.snap.valdi.schema.ValdiUntypedClass { *; }
-keep,allowobfuscation @interface com.snap.valdi.schema.ValdiEnum { *; }
-keep,allowobfuscation @interface com.snap.valdi.schema.ValdiFunctionClass { *; }
-keep,allowobfuscation @interface com.snap.valdi.schema.ValdiOptionalMethod { *; }

# Keep any method and field declared with a Valdi annotation
-keepclassmembers,includedescriptorclasses class * {
  @com.snap.valdi.schema.Valdi* *;
}

# Keep any class or interface declared with a Valdi annotation
-keep,allowobfuscation @com.snap.valdi.schema.Valdi* class *
-keep,allowobfuscation @com.snap.valdi.schema.Valdi* interface *

# Keep methods of generated interfaces
-keepclassmembers,includedescriptorclasses,allowobfuscation @com.snap.valdi.schema.Valdi* interface * {
    <methods>;
}

# Precise Resource Shrinker requires the resource ID integers be referenced in DEX Code
# If R8 inlines this method, it will replace the integer reference with a string, leading
# to incorrect removal of resources.
-keepclassmembers,allowobfuscation class com.snap.valdi.bundle.LocalAssetLoader$Companion {
    *** valdiAssetUrlFromResID(...);
}