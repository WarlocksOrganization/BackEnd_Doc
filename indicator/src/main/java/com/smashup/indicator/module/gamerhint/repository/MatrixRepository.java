package com.smashup.indicator.module.gamerhint.repository;

import com.smashup.indicator.module.gamerhint.domain.entity.MatrixDocument;
import org.springframework.data.mongodb.repository.MongoRepository;

public interface MatrixRepository extends MongoRepository<MatrixDocument, String> {
}
