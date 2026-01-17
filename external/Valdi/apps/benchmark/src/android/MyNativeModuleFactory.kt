package com.snap.valdi.benchmark
import com.snapchat.client.valdi_core.ModuleFactory
import com.snap.valdi.modules.RegisterValdiModule
import com.snap.valdi.modules.benchmark.NativeHelperModuleFactory
import com.snap.valdi.modules.benchmark.NativeHelperModule
import com.snap.valdi.modules.benchmark.FunctionTiming
import android.os.SystemClock

@RegisterValdiModule
class NativeHelperFactoryImpl: NativeHelperModuleFactory() {

    override fun onLoadModule(): NativeHelperModule {
        return object: NativeHelperModule {

            var nativeTiming = mutableListOf<FunctionTiming>()
            
            override fun measureStrings(data: List<String>): List<String> {
                val ts = SystemClock.uptimeMillis()
                nativeTiming.add(FunctionTiming(ts.toDouble(), ts.toDouble()))
                return data
            }

            override fun measureStringMaps(data: List<Map<String, Any?>>): List<Map<String, Any?>> {
                val ts = SystemClock.uptimeMillis()
                nativeTiming.add(FunctionTiming(ts.toDouble(), ts.toDouble()))
                return data
            }
            
            override fun clearNativeFunctionTiming() {
                nativeTiming.clear()
            }

            override fun getNativeFunctionTiming(): List<FunctionTiming> {
                return nativeTiming
            }
        }
    }
}
