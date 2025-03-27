package com.smashup.indicator.module.gamerhint.service.impl;

import com.smashup.indicator.module.gamerhint.controller.dto.request.InsertDataListRequestDto;
import com.smashup.indicator.module.gamerhint.controller.dto.request.InsertDataRequestDto;
import com.smashup.indicator.module.gamerhint.domain.entity.GamerHintDocument1;
import com.smashup.indicator.module.gamerhint.repository.GamerHintRepository1;
import com.smashup.indicator.module.version.BatchCountManager;
import com.smashup.indicator.module.version.PoolManager;
import lombok.RequiredArgsConstructor;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.util.*;

@Service
@RequiredArgsConstructor
public class GamerHintService {
    // 의존성 주입
    private final GamerHintRepository1 gamerHintRepository1;
    private final PoolManager poolManager;
    private final BatchCountManager batchCountManager;

    // 상수
    private final String NO_CARD = "NO_CARD";



    // 데이터 수집
    @Transactional
    public void insertData(InsertDataListRequestDto dto) throws Exception {
        // 공통 파트
        List<Integer> cardPool = poolManager.getCardPool();
        Map<Integer, Integer> cardPoolIndex = poolManager.getCardPoolIndex();
        List<Integer> classPool = poolManager.getClassPool();


        /// documentID 재료
        String documentId = String.join("/", dto.getPatchVersion(), batchCountManager.getBatchCount()+"");

        /// 전체 직업을 관리하는 Map<documentId, Map<deckId, List<카드ID> > > 세팅.
        Map<String, Map<String, List<Integer> > > resultMap = new HashMap<>();
        // 직업코드 풀을 순회하며 최종 documentId를 생성한다.
        // 생성 후 resultMap에 집어 넣는다.
        // 가져온 값이 없으면 new HashMap<>();을 넣는다.
        for (int i = 0; i < classPool.size(); i++) {
            String finalDocumentId = String.join("/", documentId, classPool.get(i)+"");
            // class document가 있는데 못 찾으면 사고. 진짜 없는거면 새로 채울때인데, 빈맵 넣으면 됨.
            GamerHintDocument1 classMap = gamerHintRepository1.findById(finalDocumentId)
                    .orElseGet(() -> null);
            if(classMap != null){
                resultMap.putIfAbsent(finalDocumentId, classMap.getDecks());
            } else{
                resultMap.putIfAbsent(finalDocumentId, new HashMap<>());
            }
        }


        // 개별 파트 => 플레이어 단위 작업 시작.

        // dto에서 작업할 리스트 추출
        List<InsertDataRequestDto> workingList = dto.getList();
        for (InsertDataRequestDto data : workingList){
            // documentId 생성
            String dataDocumentId = String.join("/",documentId, data.getClassCode()+"");

            // documentId 에 대응되는 맵 없으면 생성.
            resultMap.putIfAbsent(dataDocumentId, new HashMap<>());
            Map<String, List<Integer>> classMap = resultMap.get(dataDocumentId);

            // round1 deck 처리 => round 0에서 뽑은 선택지 처리.
            /// 덱ID와 대응되는 리스트 없으면 생성.
            if(classMap.containsKey(NO_CARD)==false){
                List<Integer> temp = new ArrayList<>(cardPool.size() + 1); // 이렇게 초기화하면 내부는 초기화 아예 안되어있어서 size() 도 0임
                for (int i = 0; i < cardPool.size() + 1; i++) {
                    temp.add(0);
                }
                classMap.put(NO_CARD, temp);
            }

            for (int i = 0; i < data.getRound1Set().size(); i++) {
                int cardId = data.getRound1Set().get(i);    // 카드 선택
                int cardIndex = cardPoolIndex.get(cardId);  // 카드 ID 기반으로 카드 인덱스 찾기.
                int oldValue = classMap.get(NO_CARD).get(cardIndex); // set에 필요한 값 미리 부팅
                classMap.get(NO_CARD).set(cardIndex, oldValue+1);         // 맵에서 덱을 찾고. 덱의 배열에서 카드 인덱스 집계량 +1
            }
            // 디버깅 포인트. 이 덱이 플레이된 횟수인데 for문에서 더하고 있었음...플레이 횟수*3 하고 있었네.
            int oldTotalValue = classMap.get(NO_CARD).get(cardPool.size()); // 디버깅할때 편리함을 위해. oldValue 재사용 안함.
            classMap.get(NO_CARD).set(cardPool.size(), oldTotalValue+1);   // 맵에서 덱을 찾고. 덱의 배열에서 마지막 인덱스  집계량 +1

            // round2 deck 처리 => round 1에서 뽑은 선택지 처리.
            /// 덱ID 생성
            List<Integer> mergedList2 = new ArrayList<>();
            mergedList2.addAll(data.getRound1Set());
            mergedList2.addAll(data.getRound2Set());
            Collections.sort(mergedList2);
            StringBuilder stringBuilder = new StringBuilder();
            /// stringBuilder로 concat
            for (int i = 0; i < mergedList2.size()-1; i++) {
                stringBuilder.append(mergedList2.get(i)).append("/");
            }
            stringBuilder.append(mergedList2.get(mergedList2.size()-1));
            String deckId2 = stringBuilder.toString();
            stringBuilder.setLength(0); // stringBuilder 초기화


            /// 덱ID와 대응되는 배열 없으면 생성.
            if(classMap.containsKey(deckId2)==false){
                List<Integer> temp = new ArrayList<>(cardPool.size() + 1); // 이렇게 초기화하면 내부는 초기화 아예 안되어있어서 size() 도 0임
                for (int i = 0; i < cardPool.size() + 1; i++) {
                    temp.add(0);
                }
                classMap.put(deckId2, temp);
            }


            for (int i = 0; i < data.getRound2Set().size(); i++) {
                int cardId = data.getRound2Set().get(i);    // 카드 선택
                int cardIndex = cardPoolIndex.get(cardId);  // 카드 ID 기반으로 카드 인덱스 찾기.

                int oldValue = classMap.get(deckId2).get(cardIndex); // set에 필요한 값 미리 부팅
                classMap.get(deckId2).set(cardIndex, oldValue+1);       // 맵에서 덱을 찾고. 덱의 배열에서 카드 인덱스 집계량 +1

            }
            int oldTotalValue2 = classMap.get(deckId2).get(cardPool.size()); // 디버깅할때 편리함을 위해. oldValue 재사용 안함.
            classMap.get(deckId2).set(cardPool.size(), oldTotalValue2+1);  // 맵에서 덱을 찾고. 덱의 배열에서 마지막 인덱스  집계량 +1


            // round3 deck 처리 => round 2에서 뽑은 선택지 처리.
            /// 덱ID 생성
            List<Integer> mergedList3 = new ArrayList<>();
            mergedList3.addAll(data.getRound1Set());
            mergedList3.addAll(data.getRound2Set());
            mergedList3.addAll(data.getRound3Set());
            Collections.sort(mergedList3);

            /// stringBuilder로 concat
            for (int i = 0; i < mergedList3.size()-1; i++) {
                stringBuilder.append(mergedList3.get(i)).append("/");
            }
            stringBuilder.append(mergedList3.get(mergedList3.size()-1));
            String deckId3 = stringBuilder.toString();
            stringBuilder.setLength(0); // stringBuilder 초기화


            /// 덱ID와 대응되는 배열 없으면 생성.
            if(classMap.containsKey(deckId3)==false){
                List<Integer> temp = new ArrayList<>(cardPool.size() + 1); // 이렇게 초기화하면 내부는 초기화 아예 안되어있어서 size() 도 0임
                for (int i = 0; i < cardPool.size() + 1; i++) {
                    temp.add(0);
                }
                classMap.put(deckId3, temp);
            }


            for (int i = 0; i < data.getRound3Set().size(); i++) {
                int cardId = data.getRound3Set().get(i);    // 카드 선택
                int cardIndex = cardPoolIndex.get(cardId);  // 카드 ID 기반으로 카드 인덱스 찾기.

                int oldValue = classMap.get(deckId3).get(cardIndex); // set에 필요한 값 미리 부팅
                classMap.get(deckId3).set(cardIndex, oldValue+1);       // 맵에서 덱을 찾고. 덱의 배열에서 카드 인덱스 집계량 +1

            }
            int oldTotalValue3 = classMap.get(deckId3).get(cardPool.size()); // 디버깅할때 편리함을 위해. oldValue 재사용 안함.
            classMap.get(deckId3).set(cardPool.size(), oldTotalValue3+1);  // 맵에서 덱을 찾고. 덱의 배열에서 마지막 인덱스  집계량 +1

        }
        // 개별 파트 종료 => resultMap 순회하면서 Map<deckId, 카드풀[]> 고르고 덱별로 insert하기.

        // resultMap 순회하면서 각각의 classMap을 GamerHintDocument1에 저장해서. 각각에 대해서
        for (String classMapId: resultMap.keySet()) {

            GamerHintDocument1 gamerHintDocument1 = GamerHintDocument1.builder()
                    .id(classMapId)
                    .cardPool(cardPool)
                    .decks(resultMap.get(classMapId))
                    .build();

            gamerHintRepository1.save(gamerHintDocument1);
        }
        // GamerHintRepository1.save(각각의 GamerHintDocument1) 해버리기

    }


}
