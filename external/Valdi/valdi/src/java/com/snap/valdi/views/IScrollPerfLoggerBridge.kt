package com.snap.valdi.views

import com.snap.valdi.callable.ValdiFunction
import com.snap.valdi.utils.ValdiMarshallable
import com.snap.valdi.utils.ValdiMarshaller
import com.snap.valdi.utils.InternedString

interface IScrollPerfLoggerBridge: ValdiMarshallable {
    fun resume()

    fun pause(cancelLogging: Boolean)

    override fun pushToMarshaller(marshaller: ValdiMarshaller) = IScrollPerfLoggerBridge.marshall(this, marshaller)

    companion object {

        private val nativeInstanceProperty = InternedString.create("\$nativeInstance")
        private val resumeProperty = InternedString.create("resume")
        private val pauseProperty = InternedString.create("pause")

        fun marshall(instance: IScrollPerfLoggerBridge, marshaller: ValdiMarshaller): Int {
            val objectIndex = marshaller.pushMap(5)
            marshaller.putMapPropertyFunction(resumeProperty, objectIndex, object: ValdiFunction {
                override fun perform(marshaller: ValdiMarshaller): Boolean {
                    instance.resume()
                    marshaller.pushUndefined()
                    return true
                }
            })

            marshaller.putMapPropertyFunction(pauseProperty, objectIndex, object: ValdiFunction {
                override fun perform(marshaller: ValdiMarshaller): Boolean {
                    instance.pause(marshaller.getBoolean(0))
                    marshaller.pushUndefined()
                    return true
                }
            })

            marshaller.putMapPropertyOpaque(nativeInstanceProperty, objectIndex, instance)

            return objectIndex
        }

        fun unmarshall(marshaller: ValdiMarshaller, objectIndex: Int): IScrollPerfLoggerBridge {
            return marshaller.getMapPropertyOpaqueCasted<IScrollPerfLoggerBridge>(nativeInstanceProperty, objectIndex)
        }
    }
}