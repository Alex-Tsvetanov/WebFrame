package com.snap.valdi.callable

import androidx.annotation.Keep
import com.snap.valdi.exceptions.ValdiFatalException
import com.snap.valdi.utils.arrayMap
import java.lang.reflect.Constructor
import java.lang.reflect.Method

@Keep
class ValdiFunctionTrampoline {

    companion object {
        @JvmStatic
        private fun resolveInvokeMethod(cls: Class<*>, arity: Int): Method {
            val methods = cls.declaredMethods
            for (method in methods) {
                if (method.returnType != Object::class.java) {
                    continue
                }

                val parameterTypes = method.parameterTypes
                if (parameterTypes.size != arity) {
                    continue
                }
                for (parameterType in parameterTypes) {
                    if (parameterType != Object::class.java) {
                        continue
                    }
                }

                return method
            }

            ValdiFatalException.handleFatal("Could not resolve invoke function for Class ${cls} with arity ${arity}")
        }

        @JvmStatic
        private fun resolveConstructor(cls: Class<*>): Constructor<*> {
            return cls.getDeclaredConstructor(Long::class.java, Long::class.java)
        }

        /**
         * This function is called by native code to resolve each Function type we support,
         * and the Java Method to use to invoke the functions.
         */
        @JvmStatic
        @Keep
        fun getFunctionClasses(): Array<Any?> {
            val functions = arrayOf(
                Pair(Function0::class.java, BridgeFunction0Impl::class.java),
                Pair(Function1::class.java, BridgeFunction1Impl::class.java),
                Pair(Function2::class.java, BridgeFunction2Impl::class.java),
                Pair(Function3::class.java, BridgeFunction3Impl::class.java),
                Pair(Function4::class.java, BridgeFunction4Impl::class.java),
                Pair(Function5::class.java, BridgeFunction5Impl::class.java),
                Pair(Function6::class.java, BridgeFunction6Impl::class.java),
                Pair(Function7::class.java, BridgeFunction7Impl::class.java),
                Pair(Function8::class.java, BridgeFunction8Impl::class.java),
                Pair(Function9::class.java, BridgeFunction9Impl::class.java),
                Pair(Function10::class.java, BridgeFunction10Impl::class.java),
                Pair(Function11::class.java, BridgeFunction11Impl::class.java),
                Pair(Function12::class.java, BridgeFunction12Impl::class.java),
                Pair(Function13::class.java, BridgeFunction13Impl::class.java),
                Pair(Function14::class.java, BridgeFunction14Impl::class.java),
                Pair(Function15::class.java, BridgeFunction15Impl::class.java),
                Pair(Function16::class.java, BridgeFunction16Impl::class.java),
            )

            var index = 0
            val invokerAndConstructor = functions.arrayMap {
                val out = Pair(resolveInvokeMethod(it.first, index), resolveConstructor(it.second))
                index++
                out
            }

            val output = ArrayList<Any?>()

            for (i in functions.indices) {
                output.add(functions[i].first.name)
                output.add(functions[i].first)
                output.add(functions[i].second)
                output.add(invokerAndConstructor[i].first)
                output.add(invokerAndConstructor[i].second)
            }

            return output.toArray()
        }

        /**
         * Each function below is used by the Functiom implementations to forward a Java call
         * to native. At the native level, the trampoline and value function will be unwrapped,
         * and the given java parameters will be forwarded to the trampoline so that they can be converted.
         */

        @JvmStatic
        external fun nativePerform0(trampolineHandle: Long,
                                    valueFunctionHandle: Long): Any?

        @JvmStatic
        external fun nativePerform1(trampolineHandle: Long,
                                    valueFunctionHandle: Long,
                                    p0: Any?): Any?

        @JvmStatic
        external fun nativePerform2(trampolineHandle: Long,
                                    valueFunctionHandle: Long,
                                    p0: Any?,
                                    p1: Any?): Any?

        @JvmStatic
        external fun nativePerform3(trampolineHandle: Long,
                                    valueFunctionHandle: Long,
                                    p0: Any?,
                                    p1: Any?,
                                    p2: Any?): Any?

        @JvmStatic
        external fun nativePerform4(trampolineHandle: Long,
                                    valueFunctionHandle: Long,
                                    p0: Any?,
                                    p1: Any?,
                                    p2: Any?,
                                    p3: Any?): Any?

        @JvmStatic
        external fun nativePerform5(trampolineHandle: Long,
                                    valueFunctionHandle: Long,
                                    p0: Any?,
                                    p1: Any?,
                                    p2: Any?,
                                    p3: Any?,
                                    p4: Any?): Any?

        @JvmStatic
        external fun nativePerform6(trampolineHandle: Long,
                                    valueFunctionHandle: Long,
                                    p0: Any?,
                                    p1: Any?,
                                    p2: Any?,
                                    p3: Any?,
                                    p4: Any?,
                                    p5: Any?): Any?

        @JvmStatic
        external fun nativePerform7(trampolineHandle: Long,
                                    valueFunctionHandle: Long,
                                    p0: Any?,
                                    p1: Any?,
                                    p2: Any?,
                                    p3: Any?,
                                    p4: Any?,
                                    p5: Any?,
                                    p6: Any?): Any?

        @JvmStatic
        external fun nativePerform8(trampolineHandle: Long,
                                    valueFunctionHandle: Long,
                                    p0: Any?,
                                    p1: Any?,
                                    p2: Any?,
                                    p3: Any?,
                                    p4: Any?,
                                    p5: Any?,
                                    p6: Any?,
                                    p7: Any?): Any?

        @JvmStatic
        external fun nativePerform9(trampolineHandle: Long,
                                    valueFunctionHandle: Long,
                                    p0: Any?,
                                    p1: Any?,
                                    p2: Any?,
                                    p3: Any?,
                                    p4: Any?,
                                    p5: Any?,
                                    p6: Any?,
                                    p7: Any?,
                                    p8: Any?): Any?

        @JvmStatic
        external fun nativePerform10(trampolineHandle: Long,
                                     valueFunctionHandle: Long,
                                     p0: Any?,
                                     p1: Any?,
                                     p2: Any?,
                                     p3: Any?,
                                     p4: Any?,
                                     p5: Any?,
                                     p6: Any?,
                                     p7: Any?,
                                     p8: Any?,
                                     p9: Any?): Any?

        @JvmStatic
        external fun nativePerform11(trampolineHandle: Long,
                                     valueFunctionHandle: Long,
                                     p0: Any?,
                                     p1: Any?,
                                     p2: Any?,
                                     p3: Any?,
                                     p4: Any?,
                                     p5: Any?,
                                     p6: Any?,
                                     p7: Any?,
                                     p8: Any?,
                                     p9: Any?,
                                     p10: Any?): Any?

        @JvmStatic
        external fun nativePerform12(trampolineHandle: Long,
                                     valueFunctionHandle: Long,
                                     p0: Any?,
                                     p1: Any?,
                                     p2: Any?,
                                     p3: Any?,
                                     p4: Any?,
                                     p5: Any?,
                                     p6: Any?,
                                     p7: Any?,
                                     p8: Any?,
                                     p9: Any?,
                                     p10: Any?,
                                     p11: Any?): Any?

        @JvmStatic
        external fun nativePerform13(trampolineHandle: Long,
                                     valueFunctionHandle: Long,
                                     p0: Any?,
                                     p1: Any?,
                                     p2: Any?,
                                     p3: Any?,
                                     p4: Any?,
                                     p5: Any?,
                                     p6: Any?,
                                     p7: Any?,
                                     p8: Any?,
                                     p9: Any?,
                                     p10: Any?,
                                     p11: Any?,
                                     p12: Any?): Any?

        @JvmStatic
        external fun nativePerform14(trampolineHandle: Long,
                                     valueFunctionHandle: Long,
                                     p0: Any?,
                                     p1: Any?,
                                     p2: Any?,
                                     p3: Any?,
                                     p4: Any?,
                                     p5: Any?,
                                     p6: Any?,
                                     p7: Any?,
                                     p8: Any?,
                                     p9: Any?,
                                     p10: Any?,
                                     p11: Any?,
                                     p12: Any?,
                                     p13: Any?): Any?

        @JvmStatic
        external fun nativePerform15(trampolineHandle: Long,
                                     valueFunctionHandle: Long,
                                     p0: Any?,
                                     p1: Any?,
                                     p2: Any?,
                                     p3: Any?,
                                     p4: Any?,
                                     p5: Any?,
                                     p6: Any?,
                                     p7: Any?,
                                     p8: Any?,
                                     p9: Any?,
                                     p10: Any?,
                                     p11: Any?,
                                     p12: Any?,
                                     p13: Any?,
                                     p14: Any?): Any?

        @JvmStatic
        external fun nativePerform16(trampolineHandle: Long,
                                     valueFunctionHandle: Long,
                                     p0: Any?,
                                     p1: Any?,
                                     p2: Any?,
                                     p3: Any?,
                                     p4: Any?,
                                     p5: Any?,
                                     p6: Any?,
                                     p7: Any?,
                                     p8: Any?,
                                     p9: Any?,
                                     p10: Any?,
                                     p11: Any?,
                                     p12: Any?,
                                     p13: Any?,
                                     p14: Any?,
                                     p15: Any?): Any?
    }

}