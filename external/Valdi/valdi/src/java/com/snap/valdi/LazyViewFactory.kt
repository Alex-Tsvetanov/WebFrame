package com.snap.valdi

import com.snap.valdi.utils.ValdiMarshallable
import com.snap.valdi.utils.ValdiMarshaller
import com.snapchat.client.valdi.utils.CppObjectWrapper

class LazyViewFactory(private val lazy: Lazy<ViewFactory>): ViewFactory, ValdiMarshallable {

    override fun pushToMarshaller(marshaller: ValdiMarshaller): Int {
        val viewFactory = lazy.value
        return marshaller.pushCppObject(viewFactory.getHandle())
    }

    override fun getHandle(): CppObjectWrapper {
        return lazy.value.getHandle()
    }

}