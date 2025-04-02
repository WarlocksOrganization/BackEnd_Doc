package com.smashup.indicator.module.gamerhint.repository;

import com.smashup.indicator.module.gamerhint.domain.entity.WinMatrixDocument;
import org.springframework.data.mongodb.repository.MongoRepository;

public interface WinMatrixRepository extends MongoRepository<WinMatrixDocument, String> {
}
