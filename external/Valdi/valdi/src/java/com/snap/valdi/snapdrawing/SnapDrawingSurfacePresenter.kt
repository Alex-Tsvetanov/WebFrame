package com.snap.valdi.snapdrawing

interface SnapDrawingSurfacePresenter {

    fun setup(surfacePresenterId: Int, listener: SnapDrawingSurfacePresenterListener?)
    fun teardown()

}
