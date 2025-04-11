package com.smashup.indicator.module.gamerhint.domain.entity;

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

@Document(collection = "winMatrices")
public class WinMatrixDocument {
    @Id
    private String id; // String으로 동적 생성 // patch+batch+매트릭스 유형
    private String type; // C | T
    private List<Integer> cardPool; // 편의성을 위해 중복 저장.
    private Map<String, List<List<Integer>> > matrixMap; // Map<deckId, List<count>>
    @Version
    private Long version;

}
