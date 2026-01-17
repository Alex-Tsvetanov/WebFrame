package com.snap.valdi.attributes

import com.snap.valdi.callable.ValdiFunction
import com.snap.valdi.exceptions.AttributeError
import com.snap.valdi.exceptions.ValdiFatalException
import com.snap.valdi.exceptions.messageWithCauses
import com.snap.valdi.utils.ValdiMarshaller

abstract class AttributePreprocessor: ValdiFunction {

    override fun perform(marshaller: ValdiMarshaller): Boolean {
        val input = marshaller.getUntyped(-1)

        try {
            val output = preprocess(input)
            marshaller.pushUntyped(output)
        } catch (attributeError: AttributeError) {
            marshaller.setError(attributeError.messageWithCauses())
        } catch (otherThrowable: Throwable) {
            ValdiFatalException.handleFatal(otherThrowable)
        }

        return true
    }

    abstract fun preprocess(input: Any?): Any?
}
