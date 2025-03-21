package com.smashup.indicator.module.version;

import org.springframework.stereotype.Component;
//import javax.annotation.PostConstruct;
import java.util.*;

@Component
public class CardPoolManager {
    // 필드 변수
    private final List<Integer> cardPool = new ArrayList<>();
    private final Map<Integer, Integer> cardPoolIndex = new HashMap<>();

    public synchronized void updateCardPoolPost(List<Integer> cardPoolDto) {
        cardPool.clear();
        cardPoolIndex.clear();

        // 새로운 카드 풀 세팅 (예제 데이터)
        // 이것도 좀 동적이어야 좋은데 // 입력을 API로 받을까?
        // 그래도 혹시 모르니까, 비밀번호 하나 달자 ㅋㅋ.. 비번을 일치하는 값만 변경 가능하게
//        cardPool.addAll(Arrays.asList(201, 202, 203, 204, 205));
        cardPool.addAll(cardPoolDto);
        Collections.sort(cardPool);
        // 인덱스 맵 업데이트
        for (int i = 0; i < cardPool.size(); i++) {
            cardPoolIndex.put(cardPool.get(i), i);

        }
    }
    public synchronized void updateCardPoolGet() {
        cardPool.clear();
        cardPoolIndex.clear();

        // 새로운 카드 풀 세팅 (예제 데이터)
//        List<Integer> cardPoolInMySQL = 서비스 호출. 디비에 있는 카드풀 가져오기.
//        cardPool.addAll(cardPoolInMySQL);
        Collections.sort(cardPool);
//        cardPool.sort();
        // 인덱스 맵 업데이트
        for (int i = 0; i < cardPool.size(); i++) {
            cardPoolIndex.put(cardPool.get(i), i);

        }
    }

    public List<Integer> getCardPool() {
        return Collections.unmodifiableList(cardPool);
    }

    public Map<Integer, Integer> getCardPoolIndex() {
        return Collections.unmodifiableMap(cardPoolIndex);
    }
}
