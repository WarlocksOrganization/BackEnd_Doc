package com.worlcok.logging.kafka

import org.apache.kafka.clients.consumer.ConsumerConfig
import org.apache.kafka.clients.producer.ProducerConfig
import org.apache.kafka.common.serialization.StringDeserializer
import org.apache.kafka.common.serialization.StringSerializer
import org.springframework.context.annotation.Bean
import org.springframework.context.annotation.Configuration
import org.springframework.kafka.annotation.EnableKafka
import org.springframework.kafka.config.ConcurrentKafkaListenerContainerFactory
import org.springframework.kafka.core.*

@EnableKafka
@Configuration
class KafkaConfig {

//    @Value("\${kafka.url}")
    private val servers: String? = "kraft-cluster-kafka-bootstrap.kafka.svc:9092"

    @Bean
    fun producerFactory(): ProducerFactory<String, String> {
        val config: MutableMap<String, Any?> = HashMap()
        config[ProducerConfig.BOOTSTRAP_SERVERS_CONFIG] = servers
        config[ProducerConfig.KEY_SERIALIZER_CLASS_CONFIG] = StringSerializer::class.java
        config[ProducerConfig.VALUE_SERIALIZER_CLASS_CONFIG] = StringSerializer::class.java
        return DefaultKafkaProducerFactory(config)
    }

    @Bean
    fun consumerFactory(): ConsumerFactory<String, String> {
        val config: MutableMap<String, Any?> = HashMap()
        config[ConsumerConfig.BOOTSTRAP_SERVERS_CONFIG] = servers
        return DefaultKafkaConsumerFactory(
            config,
            StringDeserializer(),
            StringDeserializer()
        )
    }

    @Bean
    fun kafkaTemplate(): KafkaTemplate<String, String> {
        return KafkaTemplate(producerFactory())
    }

    @Bean
    fun kafkaListenerContainerFactory(): ConcurrentKafkaListenerContainerFactory<String, String> {
        val factory = ConcurrentKafkaListenerContainerFactory<String, String>()
        factory.consumerFactory = consumerFactory()
        return factory
    }
}