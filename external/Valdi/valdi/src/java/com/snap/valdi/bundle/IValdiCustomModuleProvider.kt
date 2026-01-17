package com.snap.valdi.bundle

import com.snap.valdi.exceptions.ValdiException

/**
 * Interface representing a custom provider for .valdimodule files.
 * When provided to the Runtime, getCustomModuleData will be called
 * to give an opportunity to the provider to return the module data that
 * should be used. This can be used to either provide more up to date
 * module data, or to provide module data for modules that are not bundled
 * in the app bundle.
 */
interface IValdiCustomModuleProvider {

    /**
     * Returns the module data (.valdimodule) file for the given module path.
     * If the module cannot be loaded because of an error, this function should throw
     * a ValdiException with the underlying error. Any other types of thrown exceptions will
     * result in a crash.
     * If the module should be loaded from the built-in modules, this function should
     * return nil.
     */
    @Throws(ValdiException::class)
    fun getCustomModuleData(path: String): ByteArray?

}
