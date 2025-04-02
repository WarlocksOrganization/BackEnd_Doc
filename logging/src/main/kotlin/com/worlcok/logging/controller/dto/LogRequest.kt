package com.worlcok.logging.controller.dto

class LogRequest {
    data class Log(
        val data: List<Any>,
        val timestamp: String,
    )
}