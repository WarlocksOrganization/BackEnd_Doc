package com.smashup.indicator.module.version.domain.entity;

import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Data;
import lombok.NoArgsConstructor;
import org.springframework.data.annotation.Id;
import org.springframework.data.annotation.Version;
import org.springframework.data.mongodb.core.mapping.Document;

import java.util.List;
import java.util.Map;

@Data
@AllArgsConstructor
@NoArgsConstructor
@Builder

@Document(collection = "versionWithPool")
public class VersionWithPoolDocument {
    @Id
    private String id; // String으로 동적 생성 // patchVersion
    private Integer patchVersionNum;

    // UpdatePoolRequestDto
    private String patchVersion;
    // cardpool, patchVersion 세팅 통합하면서 추가됨.
    /// poolManager 세팅
    private Map<Integer, List<Integer>> cardPoolMap; // Map<classCode, cardPool>
    private List<Integer> classPool;
    private List<Integer> mapPool;
    private List<Integer> playerNumPool;
    /// cardName 추가
    private Map<Integer, String> allCardPoolMap;

}
