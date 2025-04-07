package com.smashup.indicator.module.gamerhint.service.impl;

import com.smashup.indicator.module.gamerhint.controller.dto.request.GameEndRequestDto;
import com.smashup.indicator.module.gamerhint.controller.dto.request.LogServerRequestDto;
import com.smashup.indicator.module.gamerhint.controller.dto.request.PlayerLogRequestDto;
import com.smashup.indicator.module.gamerhint.domain.entity.MatrixDocument;
import com.smashup.indicator.module.gamerhint.domain.entity.WinMatrixDocument;
import com.smashup.indicator.module.gamerhint.repository.MatrixRepository;
import com.smashup.indicator.module.gamerhint.repository.WinMatrixRepository;
import com.smashup.indicator.module.version.PoolManager;
import com.smashup.indicator.module.version.service.impl.VersionService;
import lombok.RequiredArgsConstructor;
import org.springframework.dao.OptimisticLockingFailureException;
import org.springframework.retry.annotation.Backoff;
import org.springframework.retry.annotation.Retryable;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.util.*;

@Service
@RequiredArgsConstructor
public class PseudoCode {
    // 의존성 주입
    private final GamerHintMatrixSubService gamerHintMatrixSubService;
    private final MatrixRepository matrixRepository;
    private final WinMatrixRepository winMatrixRepository;
    private final PoolManager poolManager;
    private final VersionService versionService;


    // 지표 전달하기.
    @Transactional
    public Map<Integer, double[]> cardOpen(List<Integer> decks, List<Integer> openCards, MatrixDocument doc, int classCode) throws Exception {
//        MatrixDocument doc = MatrixDocument.builder().build();
        // 리턴 타입 정의
        // Map<openCardId, [scoreMean, rank]>
        Map<Integer, double[]> results = new HashMap<>();


        // 카드풀 합치기
        List<Integer> mergedList = new ArrayList<>();
        mergedList.addAll(decks);
        mergedList.addAll(openCards);

        // 교체 가능한 카드풀 생성
        List<Integer> cardPool = doc.getCardPool();
        Set<Integer> replaceCardPool = new HashSet<>(cardPool);
        replaceCardPool.removeAll(mergedList);

        // 매트릭스 선택
        Map<String, List<List<Integer>>> matrixMap = doc.getMatrixMap();
        String matrixId = classCode+"/-1/-1";
        List<List<Integer>> matrix = matrixMap.get(matrixId);

        // 인덱스 맵 생성
        Map<Integer, Integer> cardPoolIndex = new HashMap<>();
        for (int i = 0; i < cardPool.size(); i++) {
            cardPoolIndex.put(cardPool.get(i), i);
        }

        // NO_CARD 상태일때 처리 필수!!!! => 위에서 계산해서 earlyReturn하기
        // already(row) 를 모두 Matrix.size()로 교체.
        // No_Card 일때 : 0=>1라운드 일때. => if(decks.isEmpty()==true){}
        if(decks.isEmpty()){
            // 각각의 카드에서 스코어 계산
            for(int openCard: openCards){
                // results에 저장할 int[] 생성
                double[] result = new double[2];
                // 상위 % 구할 List 생성.
                List<Double> scoreMeanList = new ArrayList<>();
                // 선택된 카드와 페어링해서 스코어 계산
                List<Double> scoreList = new ArrayList<>();
                for(int already: new int[] {matrix.size()-1}){
                    // 자기랑 같은 카드 패스
                    if (already == openCard) {
                        continue;
                    }
                    // 커플링 스코어 계산
                    int row = already;
                    int col = cardPoolIndex.get(openCard);
                    int up = matrix.get(row).get(col);
                    int down = matrix.get(row).get(row);
                    double score;
                    // 구조상 나올일은 없지만 예외처리.
                    if(down==0){
                        score = 0;
                    } else{
                        score = up/down;
                    }
                    scoreList.add(score);
                }
                // 스코어 평균 구하기
                double scoreSum = 0;
                for( double score : scoreList){
                    scoreSum += score;
                }

                double scoreMean;
                // 구조상 나올일은 없지만 예외처리.
                if (scoreList.isEmpty()) {
                    scoreMean = 0;
                } else{
                    scoreMean = scoreSum / scoreList.size();
                }
                scoreMeanList.add(scoreMean);
                result[0] = scoreMean;
                /// 교체가능 풀에서 순위 계산. 위에 로직 돌리면됨.
                // 대신에 선택된 카드를 교체가능 풀에서 하나씩 선택해야 함.
                for (Integer replaceCard :replaceCardPool) {
                    // 선택된 카드와 페어링해서 스코어 계산
                    List<Double> localScoreList = new ArrayList<>();
                    for(int already: new int[] {matrix.size()-1}){
                        // 자기랑 같은 카드 패스 => 구조상 나올일 없음.
                        if (already == replaceCard) {
                            continue;
                        }
                        // 커플링 스코어 계산
                        int row = already;
                        int col = cardPoolIndex.get(replaceCard);
                        int up = matrix.get(row).get(col);
                        int down = matrix.get(row).get(row);
                        double score;
                        // 구조상 나올일은 없지만 예외처리.
                        if(down==0){
                            score = 0;
                        } else{
                            score = up/down;
                        }
                        localScoreList.add(score);
                    }
                    // 스코어 평균 구하기
                    double localScoreSum = 0;
                    for( double score : localScoreList){
                        localScoreSum += score;
                    }

                    double localScoreMean;
                    // 구조상 나올일은 없지만 예외처리.
                    if (localScoreList.isEmpty()) {
                        localScoreMean = 0;
                    } else{
                        localScoreMean = localScoreSum / localScoreList.size();
                    }
                    scoreMeanList.add(localScoreMean);
                }
                // 교체가능 풀도 다 계산됨. 정렬하고 이진탐색으로 상위 몇%인지 계산하기.
                Collections.sort(scoreMeanList);
                int index = Collections.binarySearch(scoreMeanList, scoreMean);
                int totalSize = scoreMeanList.size(); // 0일때 예외처리 해야되나.
                double rank = totalSize-index/totalSize;
                result[1] = rank;
                results.put(openCard, result);
            }
            return results;
        }
        else{
            // 각각의 카드에서 스코어 계산
            for(int openCard: openCards){
                // results에 저장할 int[] 생성
                double[] result = new double[2];
                // 상위 % 구할 List 생성.
                List<Double> scoreMeanList = new ArrayList<>();
                // 선택된 카드와 페어링해서 스코어 계산
                List<Double> scoreList = new ArrayList<>();
                for(int already: mergedList){
                    // 자기랑 같은 카드 패스
                    if (already == openCard) {
                        continue;
                    }
                    // 커플링 스코어 계산
                    int row = cardPoolIndex.get(already);
                    int col = cardPoolIndex.get(openCard);
                    int up = matrix.get(row).get(col);
                    int down = matrix.get(row).get(row);
                    double score;
                    // 구조상 나올일은 없지만 예외처리.
                    if(down==0){
                        score = 0;
                    } else{
                        score = up/down;
                    }
                    scoreList.add(score);
                }
                // 스코어 평균 구하기
                double scoreSum = 0;
                for( double score : scoreList){
                    scoreSum += score;
                }

                double scoreMean;
                // 구조상 나올일은 없지만 예외처리.
                if (scoreList.isEmpty()) {
                    scoreMean = 0;
                } else{
                    scoreMean = scoreSum / scoreList.size();
                }
                scoreMeanList.add(scoreMean);
                result[0] = scoreMean;
                /// 교체가능 풀에서 순위 계산. 위에 로직 돌리면됨.
                // 대신에 선택된 카드를 교체가능 풀에서 하나씩 선택해야 함.
                for (Integer replaceCard :replaceCardPool) {
                    // 선택된 카드와 페어링해서 스코어 계산
                    List<Double> localScoreList = new ArrayList<>();
                    for(int already: mergedList){
                        // 자기랑 같은 카드 패스 => 구조상 나올일 없음.
                        if (already == replaceCard) {
                            continue;
                        }
                        // 커플링 스코어 계산
                        int row = cardPoolIndex.get(already);
                        int col = cardPoolIndex.get(replaceCard);
                        int up = matrix.get(row).get(col);
                        int down = matrix.get(row).get(row);
                        double score;
                        // 구조상 나올일은 없지만 예외처리.
                        if(down==0){
                            score = 0;
                        } else{
                            score = up/down;
                        }
                        localScoreList.add(score);
                    }
                    // 스코어 평균 구하기
                    double localScoreSum = 0;
                    for( double score : localScoreList){
                        localScoreSum += score;
                    }

                    double localScoreMean;
                    // 구조상 나올일은 없지만 예외처리.
                    if (localScoreList.isEmpty()) {
                        localScoreMean = 0;
                    } else{
                        localScoreMean = localScoreSum / localScoreList.size();
                    }
                    scoreMeanList.add(localScoreMean);
                }
                // 교체가능 풀도 다 계산됨. 정렬하고 이진탐색으로 상위 몇%인지 계산하기.
                Collections.sort(scoreMeanList);
                int index = Collections.binarySearch(scoreMeanList, scoreMean);
                int totalSize = scoreMeanList.size(); // 0일때 예외처리 해야되나.
                double rank = totalSize-index/totalSize;
                result[1] = rank;
                results.put(openCard, result);
            }
            return results;
        }


    }


}
