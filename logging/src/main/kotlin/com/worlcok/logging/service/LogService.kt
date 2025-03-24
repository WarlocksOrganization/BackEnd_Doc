package com.worlcok.logging.service

import com.fasterxml.jackson.databind.ObjectMapper
import com.worlcok.logging.controller.dto.LogRequest
import org.json.JSONObject
import org.springframework.kafka.core.KafkaTemplate
import org.springframework.stereotype.Service

@Service
class LogService (
    private val kafkaTemplate: KafkaTemplate<String, String>,
) {

    fun sendLog(log: LogRequest.Log) {
        var om =  ObjectMapper();
        log.data.forEach { it ->
            var data = om.writeValueAsString(it)
            sendToKafka(data)
        }
    }

    fun sendToKafka(data:String) {
        val jsonData = JSONObject(data)
        kafkaTemplate.send("logging", jsonData.toString())

//        when(jsonData["eventType"]) {
//            "RoomEnter" ->
//
//        }
    }
}