From https://bellard.org/quickjs initially forked off at version 2019-08-18

MODIFICATIONS:
 - Added cast fixes
 - Added JS_IsArrayBuffer()
 - Added JS_IsObjectOfClass()
 - Added JS_FreePropertyEnum()
 - Added JSTypedArrayType, JS_GetTypedArrayType(), JS_GetTypedArrayData(), and JS_NewTypedArrayCopy()
 - Added JS_SetCStackTopPointer
 - Merged upstream 2019-09-01
 - Merged upstream 2019-09-18
 - Merged upstream 2019-10-27
 - Merged upstream 2019-12-21
 - Added JS_IsInGCPhase (since JS_IsInGCSweep was removed in 2019-12-21)
 - Merged upstream 2020-01-05
 - Merged upstream 2020-01-19
 - Disabled unused-function warnings in libbf.c
 - Merged upstream 2020-03-16
 - Merged upstream 2020-04-12
 - Merged upstream 2020-07-05
 - Fix up JS_SetCStackTopPointer
 - Merged upstream 2020-09-06
 - Merged upstream 2020-11-08
 - Merged upstream 2021-03-27
