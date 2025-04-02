package com.smashup.indicator.module.version;

import com.smashup.indicator.module.version.controller.dto.request.UpdatePoolRequestDto;
import org.springframework.stereotype.Component;
//import javax.annotation.PostConstruct;
import java.util.*;

@Component
public class PoolManager {
    // 필드 변수
    private final List<Integer> classPool = new ArrayList<>();
    private final List<Integer> mapPool = new ArrayList<>();
    private final List<Integer> playerNumPool = new ArrayList<>();

    private final List<Integer> cardPool = new ArrayList<>();
    private final Map<Integer, Integer> cardPoolIndex = new HashMap<>();

    public synchronized void updatePoolPost(UpdatePoolRequestDto dto) {
        cardPool.clear();
        cardPoolIndex.clear();
        //====== 다른 풀도 추가=======
        classPool.clear();
        mapPool.clear();
        playerNumPool.clear();

        // 새로운 카드 풀 세팅 (예제 데이터)
        // 이것도 좀 동적이어야 좋은데 // 입력을 API로 받을까?
        // 그래도 혹시 모르니까, 비밀번호 하나 달자 ㅋㅋ.. 비번을 일치하는 값만 변경 가능하게
        cardPool.addAll(dto.getCardPool());

        classPool.addAll(dto.getClassPool());
        classPool.add(-1);
        mapPool.addAll(dto.getMapPool());
        mapPool.add(-1);
        playerNumPool.addAll(dto.getPlayerNumPool());
        playerNumPool.add(-1);
        //
        Collections.sort(cardPool);
        Collections.sort(classPool);
        Collections.sort(mapPool);
        Collections.sort(playerNumPool);
        // 인덱스 맵 업데이트
        for (int i = 0; i < cardPool.size(); i++) {
            cardPoolIndex.put(cardPool.get(i), i);
        }
    }


    public synchronized void updateCardPoolGet() {
//        cardPool.clear();
//        cardPoolIndex.clear();
//
//        // 새로운 카드 풀 세팅 (예제 데이터)
////        List<Integer> cardPoolInMySQL = 서비스 호출. 디비에 있는 카드풀 가져오기.
////        cardPool.addAll(cardPoolInMySQL);
//        Collections.sort(cardPool);
////        cardPool.sort();
//        // 인덱스 맵 업데이트
//        for (int i = 0; i < cardPool.size(); i++) {
//            cardPoolIndex.put(cardPool.get(i), i);
//
//        }
    }

    public List<Integer> getCardPool() {
        return Collections.unmodifiableList(cardPool);
    }
    public List<Integer> getClassPool() {
        return Collections.unmodifiableList(classPool);
    }
    public List<Integer> getPlayerNumPool() {
        return Collections.unmodifiableList(playerNumPool);
    }
    public List<Integer> getMapPool() {
        return Collections.unmodifiableList(mapPool);
    }
    public Map<Integer, Integer> getCardPoolIndex() {
        return Collections.unmodifiableMap(cardPoolIndex);
    }
}
