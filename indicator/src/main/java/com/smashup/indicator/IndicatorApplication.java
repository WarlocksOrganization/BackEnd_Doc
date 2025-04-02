package com.smashup.indicator;

import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.retry.annotation.EnableRetry;
import org.springframework.scheduling.annotation.EnableScheduling;

@SpringBootApplication
@EnableScheduling
@EnableRetry
public class IndicatorApplication {

	public static void main(String[] args) {
//		System.out.println("🔥 앱 시작됨");

		SpringApplication.run(IndicatorApplication.class, args);
	}

}
