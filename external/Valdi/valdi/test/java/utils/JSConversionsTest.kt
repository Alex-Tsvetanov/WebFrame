package com.snap.valdi.utils

import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.Assertions.assertEquals;

import com.snap.valdi.utils.JSConversions;


internal class JSConversionsTest {

    @Test
    fun getParameterOrNullTest() {
        assertEquals(null, JSConversions.getParameterOrNull(arrayOf(10), 1))
        assertEquals(10, JSConversions.getParameterOrNull(arrayOf(10), 0))
    }
}