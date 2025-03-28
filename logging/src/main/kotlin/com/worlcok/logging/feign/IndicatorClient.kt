package com.worlcok.logging.feign

import org.springframework.cloud.openfeign.FeignClient
import org.springframework.http.MediaType
import org.springframework.web.bind.annotation.PostMapping
import org.springframework.web.bind.annotation.RequestBody
import java.util.concurrent.CompletableFuture

@FeignClient(
    name = "indicator-service",
    url = "\${feign.indicator-service.url}"
)
interface IndicatorClient {

    @PostMapping("/internal/hints/data", consumes = [MediaType.APPLICATION_JSON_VALUE])
    fun sendIndicate(
        @RequestBody request: String,
    ) : CompletableFuture<Void>
}