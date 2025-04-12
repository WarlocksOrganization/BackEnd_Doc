package com.smashup.indicator.module.version;

import com.smashup.indicator.module.version.controller.dto.request.UpdatePoolRequestDto;
import org.springframework.stereotype.Component;
//import javax.annotation.PostConstruct;
import java.util.*;

@Component
public class PoolManager {
    // 필드 변수
    private String patchVersion = "not yet set";
    private final List<Integer> classPool = new ArrayList<>();
    private final List<Integer> mapPool = new ArrayList<>();
    private final List<Integer> playerNumPool = new ArrayList<>();
    private final Map<Integer, List<Integer>> classCardPoolMap = new HashMap<>(); // Map<classCode, classCardPool>
    private final Map<Integer, String> allCardPoolMap = new HashMap<>(); // Map<cardId, cardName>
    private final Map<Integer, Map<Integer, Integer>> classCardPoolIndexMap = new HashMap<>(); // Map<classCode, Map<classCardId, classCardPoolIndex>>
    private final Map<Integer, Integer> upgradeCardPoolMap = new HashMap<>(); // Map< 승급 강화 카드 id, 승급 카드 id> >

    public synchronized void updatePoolPost(UpdatePoolRequestDto dto) {
        // 패치 수정 안전 프로세스
        patchVersion = dto.getPatchVersion();

        classPool.clear();
        mapPool.clear();
        playerNumPool.clear();
        classPool.addAll(dto.getClassPool());
//        classPool.add(-1); // 도큐먼트를 직업별로 쪼개서 필요없어짐.
        mapPool.addAll(dto.getMapPool());
        mapPool.add(-1);
        playerNumPool.addAll(dto.getPlayerNumPool());
        playerNumPool.add(-1);

        Collections.sort(classPool);
        Collections.sort(mapPool);
        Collections.sort(playerNumPool);
        //====== 다른 풀도 추가됨=======

//        cardPool.clear();
//        classCardPoolIndex.clear();
        classCardPoolMap.clear();
        classCardPoolIndexMap.clear();
        // ===== 카드별 픽률 승률 보여줄때 편하려고 cardName까지 맵핑하는 용도.
        allCardPoolMap.clear();
        allCardPoolMap.putAll(dto.getAllCardPoolMap());
        // ===== 승급 카드, 승급 강화 카드 맵핑
        upgradeCardPoolMap.clear();
//        System.out.println(dto.getUpgradeCardPoolMap());
        upgradeCardPoolMap.putAll(dto.getUpgradeCardPoolMap());

        // 구조 잡으면서 정렬 후 바로 삽입. classCardPoolIndexMap도 생성하기.
        Map<Integer, List<Integer>> cardPoolMap = dto.getCardPoolMap();
        for(int classCode : classPool){
            // 클래스 별로 카드풀 저장
            List<Integer> classCardPool = cardPoolMap.get(classCode);
            Collections.sort(classCardPool);
            classCardPoolMap.put(classCode, classCardPool);

            // 클래스 별로 카드풀의 인덱스를 관리하는 맵 저장
            Map<Integer, Integer> classCardPoolIndex = new HashMap<>();
            for (int i = 0; i < classCardPool.size(); i++) {
                classCardPoolIndex.put(classCardPool.get(i), i);
            }
            classCardPoolIndexMap.put(classCode,classCardPoolIndex);
        }
    }


    public String getPatchVersion() {
        return patchVersion;
    }

    public Map<Integer, Integer>  getUpgradeCardPoolMap() {
        return Collections.unmodifiableMap(upgradeCardPoolMap);
    }
    public Map<Integer, String>  getAllCardPoolMap() {
        return Collections.unmodifiableMap(allCardPoolMap);
    }

    public Map<Integer, List<Integer>> getClassCardPoolMap() {
        return Collections.unmodifiableMap(classCardPoolMap);
    }

    public Map<Integer, Map<Integer, Integer>> getClassCardPoolIndexMap() {
        return Collections.unmodifiableMap(classCardPoolIndexMap);
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
}
