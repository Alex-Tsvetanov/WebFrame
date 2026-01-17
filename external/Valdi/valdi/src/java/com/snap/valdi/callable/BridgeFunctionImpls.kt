package com.snap.valdi.callable

import androidx.annotation.Keep
import com.snap.valdi.utils.CppNativeHandlePair

/**
 * This file contains all the different implementations of the Kotlin function interfaces, one for
 * each arity. Each implementation retains a native handle pair and forward them to native
 * when the function is called from Java.
 */

open class BridgeFunctionBase(trampolineHandle: Long,
                              valueFunctionHandle: Long):
    CppNativeHandlePair(trampolineHandle, valueFunctionHandle), Function<Any?> {
    val trampolineHandle: Long
        get() = this.nativeHandle1

    val valueFunctionHandle: Long
        get() = this.nativeHandle2

    }

class BridgeFunction0Impl: BridgeFunctionBase, Function0<Any?> {

    @Keep
    constructor(trampolineHandle: Long,
                valueFunctionHandle: Long): super(trampolineHandle, valueFunctionHandle)

    override fun invoke(): Any? {
        return ValdiFunctionTrampoline.nativePerform0(trampolineHandle, valueFunctionHandle)
    }
}

class BridgeFunction1Impl: BridgeFunctionBase, Function1<Any?, Any?> {

    @Keep
    constructor(trampolineHandle: Long,
                valueFunctionHandle: Long): super(trampolineHandle, valueFunctionHandle)

    override fun invoke(p1: Any?): Any? {
        return ValdiFunctionTrampoline.nativePerform1(trampolineHandle, valueFunctionHandle, p1)
    }
}

class BridgeFunction2Impl: BridgeFunctionBase, Function2<Any?, Any?, Any?> {

    @Keep
    constructor(trampolineHandle: Long,
                valueFunctionHandle: Long): super(trampolineHandle, valueFunctionHandle)

    override fun invoke(p1: Any?, p2: Any?): Any? {
        return ValdiFunctionTrampoline.nativePerform2(trampolineHandle, valueFunctionHandle, p1, p2)
    }
}

class BridgeFunction3Impl: BridgeFunctionBase, Function3<Any?, Any?, Any?, Any?> {

    @Keep
    constructor(trampolineHandle: Long,
                valueFunctionHandle: Long): super(trampolineHandle, valueFunctionHandle)

    override fun invoke(p1: Any?, p2: Any?, p3: Any?): Any? {
        return ValdiFunctionTrampoline.nativePerform3(trampolineHandle, valueFunctionHandle, p1, p2, p3)
    }
}

class BridgeFunction4Impl: BridgeFunctionBase, Function4<Any?, Any?, Any?, Any?, Any?> {

    @Keep
    constructor(trampolineHandle: Long,
                valueFunctionHandle: Long): super(trampolineHandle, valueFunctionHandle)

    override fun invoke(p1: Any?, p2: Any?, p3: Any?, p4: Any?): Any? {
        return ValdiFunctionTrampoline.nativePerform4(trampolineHandle, valueFunctionHandle, p1, p2, p3, p4)
    }
}

class BridgeFunction5Impl: BridgeFunctionBase, Function5<Any?, Any?, Any?, Any?, Any?, Any?> {

    @Keep
    constructor(trampolineHandle: Long,
                valueFunctionHandle: Long): super(trampolineHandle, valueFunctionHandle)

    override fun invoke(p1: Any?, p2: Any?, p3: Any?, p4: Any?, p5: Any?): Any? {
        return ValdiFunctionTrampoline.nativePerform5(trampolineHandle, valueFunctionHandle, p1, p2, p3, p4, p5)
    }
}

class BridgeFunction6Impl: BridgeFunctionBase, Function6<Any?, Any?, Any?, Any?, Any?, Any?, Any?> {

    @Keep
    constructor(trampolineHandle: Long,
                valueFunctionHandle: Long): super(trampolineHandle, valueFunctionHandle)

    override fun invoke(p1: Any?, p2: Any?, p3: Any?, p4: Any?, p5: Any?, p6: Any?): Any? {
        return ValdiFunctionTrampoline.nativePerform6(trampolineHandle, valueFunctionHandle, p1, p2, p3, p4, p5, p6)
    }
}

class BridgeFunction7Impl: BridgeFunctionBase, Function7<Any?, Any?, Any?, Any?, Any?, Any?, Any?, Any?> {

    @Keep
    constructor(trampolineHandle: Long,
                valueFunctionHandle: Long): super(trampolineHandle, valueFunctionHandle)

    override fun invoke(p1: Any?, p2: Any?, p3: Any?, p4: Any?, p5: Any?, p6: Any?, p7: Any?): Any? {
        return ValdiFunctionTrampoline.nativePerform7(trampolineHandle, valueFunctionHandle, p1, p2, p3, p4, p5, p6, p7)
    }
}

class BridgeFunction8Impl: BridgeFunctionBase, Function8<Any?, Any?, Any?, Any?, Any?, Any?, Any?, Any?, Any?> {

    @Keep
    constructor(trampolineHandle: Long,
                valueFunctionHandle: Long): super(trampolineHandle, valueFunctionHandle)

    override fun invoke(p1: Any?, p2: Any?, p3: Any?, p4: Any?, p5: Any?, p6: Any?, p7: Any?, p8: Any?): Any? {
        return ValdiFunctionTrampoline.nativePerform8(trampolineHandle, valueFunctionHandle, p1, p2, p3, p4, p5, p6, p7, p8)
    }
}

class BridgeFunction9Impl: BridgeFunctionBase, Function9<Any?, Any?, Any?, Any?, Any?, Any?, Any?, Any?, Any?, Any?> {

    @Keep
    constructor(trampolineHandle: Long,
                valueFunctionHandle: Long): super(trampolineHandle, valueFunctionHandle)

    override fun invoke(p1: Any?, p2: Any?, p3: Any?, p4: Any?, p5: Any?, p6: Any?, p7: Any?, p8: Any?, p9: Any?): Any? {
        return ValdiFunctionTrampoline.nativePerform9(trampolineHandle, valueFunctionHandle, p1, p2, p3, p4, p5, p6, p7, p8, p9)
    }
}

class BridgeFunction10Impl: BridgeFunctionBase, Function10<Any?, Any?, Any?, Any?, Any?, Any?, Any?, Any?, Any?, Any?, Any?> {

    @Keep
    constructor(trampolineHandle: Long,
                valueFunctionHandle: Long): super(trampolineHandle, valueFunctionHandle)

    override fun invoke(p1: Any?, p2: Any?, p3: Any?, p4: Any?, p5: Any?, p6: Any?, p7: Any?, p8: Any?, p9: Any?, p10: Any?): Any? {
        return ValdiFunctionTrampoline.nativePerform10(trampolineHandle, valueFunctionHandle, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10)
    }
}

class BridgeFunction11Impl: BridgeFunctionBase, Function11<Any?, Any?, Any?, Any?, Any?, Any?, Any?, Any?, Any?, Any?, Any?, Any?> {

    @Keep
    constructor(trampolineHandle: Long,
                valueFunctionHandle: Long): super(trampolineHandle, valueFunctionHandle)

    override fun invoke(p1: Any?, p2: Any?, p3: Any?, p4: Any?, p5: Any?, p6: Any?, p7: Any?, p8: Any?, p9: Any?, p10: Any?, p11: Any?): Any? {
        return ValdiFunctionTrampoline.nativePerform11(trampolineHandle, valueFunctionHandle, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11)
    }
}

class BridgeFunction12Impl: BridgeFunctionBase, Function12<Any?, Any?, Any?, Any?, Any?, Any?, Any?, Any?, Any?, Any?, Any?, Any?, Any?> {

    @Keep
    constructor(trampolineHandle: Long,
                valueFunctionHandle: Long): super(trampolineHandle, valueFunctionHandle)

    override fun invoke(p1: Any?, p2: Any?, p3: Any?, p4: Any?, p5: Any?, p6: Any?, p7: Any?, p8: Any?, p9: Any?, p10: Any?, p11: Any?, p12: Any?): Any? {
        return ValdiFunctionTrampoline.nativePerform12(trampolineHandle, valueFunctionHandle, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12)
    }
}

class BridgeFunction13Impl: BridgeFunctionBase, Function13<Any?, Any?, Any?, Any?, Any?, Any?, Any?, Any?, Any?, Any?, Any?, Any?, Any?, Any?> {

    @Keep
    constructor(trampolineHandle: Long,
                valueFunctionHandle: Long): super(trampolineHandle, valueFunctionHandle)

    override fun invoke(p1: Any?, p2: Any?, p3: Any?, p4: Any?, p5: Any?, p6: Any?, p7: Any?, p8: Any?, p9: Any?, p10: Any?, p11: Any?, p12: Any?, p13: Any?): Any? {
        return ValdiFunctionTrampoline.nativePerform13(trampolineHandle, valueFunctionHandle, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13)
    }
}

class BridgeFunction14Impl: BridgeFunctionBase, Function14<Any?, Any?, Any?, Any?, Any?, Any?, Any?, Any?, Any?, Any?, Any?, Any?, Any?, Any?, Any?> {

    @Keep
    constructor(trampolineHandle: Long,
                valueFunctionHandle: Long): super(trampolineHandle, valueFunctionHandle)

    override fun invoke(p1: Any?, p2: Any?, p3: Any?, p4: Any?, p5: Any?, p6: Any?, p7: Any?, p8: Any?, p9: Any?, p10: Any?, p11: Any?, p12: Any?, p13: Any?, p14: Any?): Any? {
        return ValdiFunctionTrampoline.nativePerform14(trampolineHandle, valueFunctionHandle, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14)
    }
}

class BridgeFunction15Impl: BridgeFunctionBase, Function15<Any?, Any?, Any?, Any?, Any?, Any?, Any?, Any?, Any?, Any?, Any?, Any?, Any?, Any?, Any?, Any?> {

    @Keep
    constructor(trampolineHandle: Long,
                valueFunctionHandle: Long): super(trampolineHandle, valueFunctionHandle)

    override fun invoke(p1: Any?, p2: Any?, p3: Any?, p4: Any?, p5: Any?, p6: Any?, p7: Any?, p8: Any?, p9: Any?, p10: Any?, p11: Any?, p12: Any?, p13: Any?, p14: Any?, p15: Any?): Any? {
        return ValdiFunctionTrampoline.nativePerform15(trampolineHandle, valueFunctionHandle, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15)
    }
}

class BridgeFunction16Impl: BridgeFunctionBase, Function16<Any?, Any?, Any?, Any?, Any?, Any?, Any?, Any?, Any?, Any?, Any?, Any?, Any?, Any?, Any?, Any?, Any?> {

    @Keep
    constructor(trampolineHandle: Long,
                valueFunctionHandle: Long): super(trampolineHandle, valueFunctionHandle)

    override fun invoke(p1: Any?, p2: Any?, p3: Any?, p4: Any?, p5: Any?, p6: Any?, p7: Any?, p8: Any?, p9: Any?, p10: Any?, p11: Any?, p12: Any?, p13: Any?, p14: Any?, p15: Any?, p16: Any?): Any? {
        return ValdiFunctionTrampoline.nativePerform16(trampolineHandle, valueFunctionHandle, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16)
    }
}