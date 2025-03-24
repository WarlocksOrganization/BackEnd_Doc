package com.worlcok.logging.controller

import com.fasterxml.jackson.databind.ObjectMapper
import com.worlcok.logging.controller.dto.LogRequest
import com.worlcok.logging.kafka.toEvent
import com.worlcok.logging.service.LogService
import org.springframework.kafka.core.KafkaTemplate
import org.springframework.web.bind.annotation.PostMapping
import org.springframework.web.bind.annotation.RequestBody
import org.springframework.web.bind.annotation.RequestMapping
import org.springframework.web.bind.annotation.RestController

@RestController
@RequestMapping("/api/log")
class LogApi(
    private val logService: LogService,
) {

    @PostMapping
    fun log(@RequestBody log: LogRequest.Log) {
        logService.sendLog(log)
    }
}