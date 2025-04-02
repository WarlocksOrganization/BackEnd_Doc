package com.worlcok.logging.kafka

import com.fasterxml.jackson.core.JsonProcessingException
import com.fasterxml.jackson.databind.ObjectMapper

fun toEvent(eventObject : Any) : String  {
    var objectMapper = ObjectMapper();

    var jsonEvent:String
    try {
        jsonEvent = objectMapper.writeValueAsString(eventObject);
    } catch (e: JsonProcessingException) {
        throw RuntimeException("카프카 json 변환 실패");
    }

    return jsonEvent;
}

fun <T> toObject(event: String, targetObject: Class<T>) :T {
    var objectMapper =  ObjectMapper();

    var resultObject: T
    try {
        resultObject = objectMapper.readValue(event, targetObject);
    } catch (e: JsonProcessingException) {
        throw RuntimeException("kafka 객체 변환 실패");
    }

    return resultObject;
}