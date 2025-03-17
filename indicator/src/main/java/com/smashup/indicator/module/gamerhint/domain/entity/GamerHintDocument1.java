package com.smashup.indicator.module.gamerhint.domain.entity;

import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Data;
import lombok.NoArgsConstructor;
import org.springframework.data.annotation.Id;
import org.springframework.data.mongodb.core.mapping.Document;

import java.util.List;
import java.util.Map;

@Data
@AllArgsConstructor
@NoArgsConstructor
@Builder

@Document(collection = "gamerHints1")
public class GamerHintDocument1 {
    @Id
    private String id; // String으로 동적 생성 // patch+batch+직업ID
    private List<Integer> cardPool; // 모든 직업이 카드풀을 공유하지만
    // 언젠가 직업별로 별도  의 카드 풀을 사용할수도 있으니까 확장성을 위해 유지
    // 유지하는 비용 낮음
    private Map<String, List<Integer>> decks; // Map<deckId, List<count>>
}
