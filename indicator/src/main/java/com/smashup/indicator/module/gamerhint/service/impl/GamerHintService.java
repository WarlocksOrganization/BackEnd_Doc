package com.smashup.indicator.module.gamerhint.service.impl;

import com.smashup.indicator.module.gamerhint.controller.dto.request.InsertDataListRequestDto;
import com.smashup.indicator.module.gamerhint.controller.dto.request.InsertDataRequestDto;
import com.smashup.indicator.module.gamerhint.repository.GamerHintRepository;
import lombok.RequiredArgsConstructor;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.util.*;

@Service
@RequiredArgsConstructor
public class GamerHintService {
    // 의존성 주입
    private final GamerHintRepository gamerHintRepository;

    // 전역 변수 참조 // 임시 나중에 다 전역변수 역할의 bean, @Component로 빼는게 좋을듯
//    private final String PATCH_VERSION = "1.0.0";
    private final int BATCH_COUNT = 1;
    private final String NO_CARD = "NO_CARD";
    // 전역 변수 : 정렬된 카드풀 배열. List<Integer:카드ID>
    private final List<Integer> cardPool = new ArrayList<>();
    // 전역 변수 : 카드 인덱싱 맵. Map<카드ID, 배열상의 인덱스> => cardPool 순회하면서 만들어야 함.
    private final Map<Integer, Integer> cardPoolIndex = new HashMap<>();

    // 데이터 수집
    @Transactional
    public void insertData(InsertDataListRequestDto dto) throws Exception {
        // 공통 파트

        /// documentID 재료
        String documentId = String.join("/", dto.getPatchVersion(), BATCH_COUNT+"");

        /// 결과 관리하는 Map<documentId, Map<deckId, 카드풀[]>>
        Map<String, Map<String, int[]>> resultMap = new HashMap<>();


        // 개별 파트 => 직업 단위 작업 시작.

        // dto에서 작업할 리스트 추출
        List<InsertDataRequestDto> workingList = dto.getList();
        for (InsertDataRequestDto data : workingList){
            // documentId 생성
            String dataDocumentId = String.join("/",documentId,data.getClassCode()+"");

            // documentId 에 대응되는 맵 없으면 생성.
            resultMap.putIfAbsent(dataDocumentId, new HashMap<>());
            Map<String, int[]> classMap = resultMap.get(dataDocumentId);

            // round1 deck 처리 => round 0에서 뽑은 선택지 처리.
            /// 덱ID와 대응되는 배열 없으면 생성.
            classMap.putIfAbsent(NO_CARD, new int[cardPool.size()+1]);

            for (int i = 0; i < data.getRound1Set().size(); i++) {
                int cardId = data.getRound1Set().get(i);    // 카드 선택
                int cardIndex = cardPoolIndex.get(cardId);  // 카드 ID 기반으로 카드 인덱스 찾기.
                classMap.get(NO_CARD)[cardIndex]++;         // 맵에서 덱을 찾고. 덱의 배열에서 카드 인덱스 집계량 +1
                classMap.get(NO_CARD)[cardPool.size()]++;   // 맵에서 덱을 찾고. 덱의 배열에서 마지막 인덱스  집계량 +1
            }

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
            classMap.putIfAbsent(deckId2, new int[cardPool.size()+1]);

            for (int i = 0; i < data.getRound2Set().size(); i++) {
                int cardId = data.getRound2Set().get(i);    // 카드 선택
                int cardIndex = cardPoolIndex.get(cardId);  // 카드 ID 기반으로 카드 인덱스 찾기.
                classMap.get(deckId2)[cardIndex]++;         // 맵에서 덱을 찾고. 덱의 배열에서 카드 인덱스 집계량 +1
                classMap.get(deckId2)[cardPool.size()]++;   // 맵에서 덱을 찾고. 덱의 배열에서 마지막 인덱스  집계량 +1
            }


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
            classMap.putIfAbsent(deckId3, new int[cardPool.size()+1]);

            for (int i = 0; i < data.getRound3Set().size(); i++) {
                int cardId = data.getRound3Set().get(i);    // 카드 선택
                int cardIndex = cardPoolIndex.get(cardId);  // 카드 ID 기반으로 카드 인덱스 찾기.
                classMap.get(deckId3)[cardIndex]++;         // 맵에서 덱을 찾고. 덱의 배열에서 카드 인덱스 집계량 +1
                classMap.get(deckId3)[cardPool.size()]++;   // 맵에서 덱을 찾고. 덱의 배열에서 마지막 인덱스  집계량 +1
            }

        }
        // 개별 파트 종료 => resultMap 순회하면서 Map<deckId, 카드풀[]> 고르고 덱별로 insert하기.





//        // entity 조회로 일관성 유지.
//        MemberEntity selectedMemberEntity = memberRepository.findById( memberEntity.getMemberId() )
//                .orElseThrow( ()-> new Exception() );
//        // 새 닉네임 set
//        selectedMemberEntity.setMemberNickname(memberEntity.getMemberNickname());
//        // member update
//        memberRepository.save(selectedMemberEntity);
    }


}
