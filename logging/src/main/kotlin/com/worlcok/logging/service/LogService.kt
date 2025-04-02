package com.worlcok.logging.service

import com.fasterxml.jackson.databind.ObjectMapper
import com.worlcok.logging.controller.dto.LogRequest
import com.worlcok.logging.feign.IndicatorClient
import org.json.JSONObject
import org.springframework.beans.factory.annotation.Value
import org.springframework.kafka.core.KafkaTemplate
import org.springframework.scheduling.annotation.Async
import org.springframework.stereotype.Service
import java.util.StringTokenizer

@Service
class LogService (
    private val kafkaTemplate: KafkaTemplate<String, String>,
    private val indicatorClient: IndicatorClient,
) {

    var om =  ObjectMapper()
    val indicatorEventType = "gameEnd"

    @Async
    fun sendLog(log: LogRequest.Log) {
        var sb: StringBuilder = StringBuilder();
        var check = false

        sb.append("{\"data\":[")

        log.data.forEach { it ->
            val jsonData = om.writeValueAsString(it)
            sendToKafka(jsonData)

            if( getEventType(jsonData).equals(indicatorEventType)) {
                sb.append(jsonData)
                sb.append(",")
                check = true
            }
        }
        if(check) {
            sb.deleteCharAt( sb.length - 1)
            sb.append("]}")
            sendToIndicator(sb.toString())
        }
    }

    fun sendToIndicator(jsonData: String) {
        indicatorClient.sendIndicate(jsonData)
    }

    fun getEventType(jsonData: String): String {
        val map: Map<String, Any> = om.readValue(jsonData, Map::class.java) as Map<String, Any>

        return map["eventType"] as String
    }
    fun sendToKafka(data:String) {
        val jsonData = JSONObject(data)
        kafkaTemplate.send("logging", jsonData.toString())
    }
}