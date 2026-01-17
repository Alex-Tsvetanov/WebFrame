package com.snap.valdi.views

interface ValdiScrollableView {

    fun onScrollSpecsChanged(contentOffsetX: Int,
                             contentOffsetY: Int,
                             contentWidth: Int,
                             contentHeight: Int,
                             animated: Boolean)

}