# Regenerating Files
Files under generated-src are built by djinni.
Djinni can be invoked through a shell script with the target module directory as the first parameter:
> sh ../../tools/run_djinni.sh ../../src/open_source/valdi_core

* Target module directory is expected to have a .djinni file
* Target module directory's .djinni file may include other files through @extern, check that these files have the correct relative path if there are "Unable to find file" errors

## Other Notes
The following generated file requires manual editing:
> open_source/valdi_core/generated-src/jni/valdi_core/NativeHTTPRequestManagerCompletion.cpp

Add the following to the header includes:
> #include "valdi_core/bindings/UndefExceptions_jni.hpp"

The following generated file requires manual editing:
> open_source/valdi_core/generated-src/objc/valdi_core/SCNValdiCoreHTTPRequestManagerCompletion+Private.mm

Add the following to the header includes:
> #include "valdi_core/bindings/UndefExceptions_objc.hpp"

