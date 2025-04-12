package com.smashup.indicator.module.version.repository;

import com.smashup.indicator.module.version.domain.entity.VersionWithPoolDocument;
import org.springframework.data.mongodb.repository.MongoRepository;

public interface VersionWithPoolRepository extends MongoRepository<VersionWithPoolDocument, String> {
}
