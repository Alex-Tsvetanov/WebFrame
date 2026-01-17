package com.snap.valdi.attributes.impl.fonts

enum class FontWeight {
    LIGHT,
    NORMAL,
    MEDIUM,
    DEMI_BOLD,
    BOLD,
    BLACK;

    companion object {
        @JvmStatic
        fun fromString(str: String): FontWeight {
            return when (str) {
                "light" -> LIGHT
                "normal" -> NORMAL
                "demi-bold" -> DEMI_BOLD
                "medium" -> MEDIUM
                "bold" -> BOLD
                "black" -> BLACK
                else -> NORMAL
            }
        }
    }

    override fun toString(): String {
        return when(this) {
            LIGHT -> "light"
            NORMAL -> "normal"
            MEDIUM -> "medium"
            DEMI_BOLD -> "demi-bold"
            BOLD -> "bold"
            BLACK -> "black"
        }
    }
}

enum class FontStyle {
    NORMAL,
    ITALIC;

    companion object {
        @JvmStatic
        fun fromString(str: String): FontStyle {
            return when (str) {
                "italic" -> ITALIC
                else -> NORMAL
            }
        }
    }

    override fun toString(): String {
        return when(this) {
            NORMAL -> "normal"
            ITALIC -> "italic"
        }
    }
}

class FontDescriptor {

    val name: String?
    val family: String?
    val weight: FontWeight
    val style: FontStyle

    constructor(
        name: String? = null,
        family: String? = null,
            weight: FontWeight? = null,
            style: FontStyle? = null) {
        this.name = name?.lowercase()
        this.family = family?.lowercase()
        this.weight = weight ?: FontWeight.NORMAL
        this.style = style ?: FontStyle.NORMAL
    }

    override fun equals(other: Any?): Boolean {
        if (other !is FontDescriptor) {
            return false
        }

        if (family != other.family) {
            return false
        }
        if (name != other.name) {
            return false
        }
        if (weight != other.weight) {
            return false
        }
        if (style != other.style) {
            return false
        }
        return true
    }

    override fun hashCode(): Int {
        var result = weight.hashCode()
        result = 31 * result + style.hashCode()
        result = 31 * result + (family?.hashCode() ?: 0)
        result = 31 * result + (name?.hashCode() ?: 0)
        return result
    }

    override fun toString(): String {
        return "${name}-${family}-${weight}-${style}"
    }
}