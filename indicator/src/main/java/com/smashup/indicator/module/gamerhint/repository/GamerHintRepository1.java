package com.smashup.indicator.module.gamerhint.repository;

import com.smashup.indicator.module.gamerhint.domain.entity.GamerHintDocument1;
import org.springframework.data.mongodb.repository.MongoRepository;

// MongoRepository<Integer, String> 에서 Integer를 Document로 바꾸어야 함.
public interface GamerHintRepository1 extends MongoRepository<GamerHintDocument1, String> {
}
