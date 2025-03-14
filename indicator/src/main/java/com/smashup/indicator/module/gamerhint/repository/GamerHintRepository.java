package com.smashup.indicator.module.gamerhint.repository;

import org.springframework.data.mongodb.repository.MongoRepository;

// MongoRepository<Integer, String> 에서 Integer를 Document로 바꾸어야 함.
public interface GamerHintRepository extends MongoRepository<Integer, String> {
}
