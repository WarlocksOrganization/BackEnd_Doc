package com.smashup.indicator.module.gamerhint.service.impl;

import com.smashup.indicator.module.gamerhint.domain.entity.MatrixDocument;
import com.smashup.indicator.module.gamerhint.repository.MatrixRepository;
import com.smashup.indicator.module.gamerhint.repository.WinMatrixRepository;
import com.smashup.indicator.module.version.PoolManager;
import com.smashup.indicator.module.version.service.impl.VersionService;
import lombok.RequiredArgsConstructor;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.util.*;

@Service
@RequiredArgsConstructor
public class PseudoCodeRefactoring {
    // 의존성 주입
    private final GamerHintMatrixSubService gamerHintMatrixSubService;
    private final MatrixRepository matrixRepository;
    private final WinMatrixRepository winMatrixRepository;
    private final PoolManager poolManager;
    private final VersionService versionService;



    public Map<Integer, double[]> cardOpen(List<Integer> decks, List<Integer> openCards, MatrixDocument doc, int classCode) throws Exception {
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


        // 각각의 선택 카드에서 스코어 계산
        for(int openCard: openCards){
            // results에 저장할 double[] 생성
            double[] result = new double[2];
            // 상위 % 구할 List 생성.
            List<Double> scoreMeanList = new ArrayList<>();
            // 오픈된 카드를 선택하고 mergedList 카드들과의 평균 스코어 계산
            double scoreMean = calculateScoreMean(mergedList, openCard, openCard, matrix, cardPoolIndex);
            scoreMeanList.add(scoreMean);
            result[0] = scoreMean;
            // 교체가능한 카드를 하나씩 선택하고 mergedList 카드들과의 평균 스코어 계산
            for (Integer replaceCard :replaceCardPool) {
                double localScoreMean = calculateScoreMean(mergedList, openCard, replaceCard, matrix, cardPoolIndex);
                scoreMeanList.add(localScoreMean);
            }
            // 교체가능 풀도 다 계산됨. 정렬하고 이진탐색으로 상위 몇%인지 계산하기.
            // 이진 탐색은 중복값 이슈가 있으니까, 차라리 보다 좋은 카드의 개수 구하기
//            Collections.sort(scoreMeanList);
//            int index = Collections.binarySearch(scoreMeanList, scoreMean);
            int greaterCount = 0;
            for(double scoreMeanEle : scoreMeanList){
                // 현재 카드 보다 큰거 카운팅
                if(scoreMeanEle > scoreMean){
                    greaterCount++;
                }
            }
            int totalSize = scoreMeanList.size(); // 0일때 예외처리 해야되나.
            double rank = (double) greaterCount/ (double) totalSize;
            // rank가 높을수록, 교체하면 이득볼 확률 높음.
            // rank가 낮을수록, 교체하면 손해볼 확률 높음.
            result[1] = rank;
            results.put(openCard, result);
        }
        return results;



    }

    public double calculateScoreMean(List<Integer> mergedList, int openCard, int selectCard, List<List<Integer>> matrix, Map<Integer, Integer> cardPoolIndex) throws Exception {
        // 선택된 카드와 페어링해서 스코어 계산
        List<Double> scoreList = new ArrayList<>();
        for(int already: mergedList){
            // 자기랑 같은 카드 패스
            if (already == openCard) {
                continue;
            }
            // 커플링 스코어 계산
            int row = cardPoolIndex.get(already);
            int col = cardPoolIndex.get(selectCard);
            int up = matrix.get(row).get(col);
            int down = matrix.get(row).get(row);
            double score;
            // 구조상 나올일은 없지만 예외처리.
            if(down==0){
                score = 0;
            } else{
                score = (double) up/ (double) down;
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
            scoreMean = (double) scoreSum / (double) scoreList.size();
        }

        return scoreMean;
    }

}
