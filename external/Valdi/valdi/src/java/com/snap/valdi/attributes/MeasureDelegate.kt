package com.snap.valdi.attributes

interface MeasureDelegate {
    /**
     * Called whenever an element needs to be measured.
     * The ViewLayoutAttributes will contain all attributes that were set on the element
     * for which "invalidateLayoutOnChange" was set as true during attributes binding.
     */
    fun onMeasure(attributes: ViewLayoutAttributes,
                  widthMeasureSpec: Int,
                  heightMeasureSpec: Int,
                  isRightToLeft: Boolean): MeasuredSize
}