package com.snap.valdi

interface DebugMessagePresenter {
    enum class Level {
        INFO, ERROR
    }

    fun presentDebugMessage(level: Level, str: String)
}