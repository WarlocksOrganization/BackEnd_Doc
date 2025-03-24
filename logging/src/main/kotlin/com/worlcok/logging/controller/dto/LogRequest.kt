package com.worlcok.logging.controller.dto

class LogRequest {
    data class Log(
        val serverInfo: String,
        val data: List<Any>,
        val sendTime: String,
    )
}