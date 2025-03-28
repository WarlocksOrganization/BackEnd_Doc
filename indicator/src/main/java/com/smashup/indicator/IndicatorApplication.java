package com.smashup.indicator;

import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.retry.annotation.EnableRetry;

@SpringBootApplication
@EnableRetry
public class IndicatorApplication {

	public static void main(String[] args) {
		SpringApplication.run(IndicatorApplication.class, args);
	}

}
