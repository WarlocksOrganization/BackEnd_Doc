package com.smashup.indicator.module.gamerhint.service.impl;

import com.smashup.indicator.module.gamerhint.controller.dto.request.PlayerLogRequestDto;
import com.smashup.indicator.module.gamerhint.domain.entity.MatrixDocument;
import com.smashup.indicator.module.gamerhint.domain.entity.WinMatrixDocument;
import com.smashup.indicator.module.version.PoolManager;
import lombok.RequiredArgsConstructor;
import org.springframework.data.domain.Sort;
import org.springframework.data.mongodb.core.MongoTemplate;
import org.springframework.data.mongodb.core.query.Criteria;
import org.springframework.data.mongodb.core.query.Query;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.util.*;

@Service
@RequiredArgsConstructor
public class GamerHintMatrixSubService {
    // 의존성 주입
//    private final MatrixRepository matrixRepository;
    private final PoolManager poolManager;
    private final MongoTemplate mongoTemplate;


    // 현재 batchCount의 document class개수*2개  가져오기.
    @Transactional
    public List<MatrixDocument> getDocumentByBatch(String inputPatchVersion, Integer batchCount) throws Exception {
        Query query = new Query();
        String regex1 = String.join("/", inputPatchVersion,batchCount+"","");
        query.addCriteria(Criteria.where("_id").regex("^"+regex1));
        List<MatrixDocument> results = mongoTemplate.find(query, MatrixDocument.class);

        // 만약 C, T 중 하나만 존재하는 경우에 대한 처리
        if (results.size() == 1) {
            // 로그 추가 or 예외 처리 (C, T가 항상 있어야 하는데 1개만 존재할 경우 대비)
            // System.out.println("경고: " + lastBatchCount + "의 C 또는 T 중 하나가 누락됨");
        }
        return results;
    }

    // 현재 batchCount의 document 2개 가져오기.
    @Transactional
    public List<WinMatrixDocument> getWinDocumentByBatch(String inputPatchVersion, Integer batchCount) throws Exception {
        Query query = new Query();
        String regex1 = String.join("/", inputPatchVersion,batchCount+"","");
        query.addCriteria(Criteria.where("_id").regex("^"+regex1));
        List<WinMatrixDocument> results = mongoTemplate.find(query, WinMatrixDocument.class);

        // 만약 C, T 중 하나만 존재하는 경우에 대한 처리
        if (results.size() == 1) {
            // 로그 추가 or 예외 처리 (C, T가 항상 있어야 하는데 1개만 존재할 경우 대비)
            // System.out.println("경고: " + lastBatchCount + "의 C 또는 T 중 하나가 누락됨");
        }
        return results;
    }

    // 이 버전의 도큐먼트가 있는지 확인하기.
    @Transactional
    public boolean haveDocument(String inputPatchVersion) throws Exception {
        // 이 버전의 도큐먼트가 있는지 확인하기.
        String regex1 = "^"+inputPatchVersion;
        Query batchCountCheckQuery = new Query();
        batchCountCheckQuery.addCriteria(Criteria.where("_id").regex(regex1));
        batchCountCheckQuery.with(Sort.by(Sort.Direction.DESC, "_id"));
        batchCountCheckQuery.limit(1);

        boolean result = mongoTemplate.exists(batchCountCheckQuery, MatrixDocument.class);
        return result;
    }

    // 마지막 배치 (가장 최신 배치) load
    @Transactional
//    public List<MatrixDocument> getLatestDocument(String inputPatchVersion) throws Exception {
    public Integer getLatestBatchCount(String inputPatchVersion) throws Exception {
        // 1. 가장 마지막 batchCount 하나만 가져옴 => batchCount 안전하게 가져오기
        String regex1 = "^"+inputPatchVersion;
        Query batchCountCheckQuery = new Query();
        batchCountCheckQuery.addCriteria(Criteria.where("_id").regex(regex1));
//        batchCountCheckQuery.with(Sort.by(Sort.Direction.DESC, "_id"));
        batchCountCheckQuery.with(Sort.by(Sort.Direction.DESC, "batchCount"));
        batchCountCheckQuery.limit(1);

        MatrixDocument lastBatchDoc = mongoTemplate.findOne(batchCountCheckQuery, MatrixDocument.class);

//        // 검색결과가 없음 => inputPatchVersion 에 해당되는 Document 가 없음.
//        if (lastBatchDoc == null) {
//
//            return null; // 예외처리 필요 => 호출 위치상 inputPatchVersion 에 해당되는 Document가 있을때만 호출됨.
//        }

        // 2. 마지막 batchCount를 추출
        return lastBatchDoc.getBatchCount();

//        // 3. 마지막 batchCount에 해당하는 C, T를 가져옴
//        Query finalQuery = new Query();
//        String regex2 = String.join("/", regex1,lastBatchCount,"");
//
//        finalQuery.addCriteria(Criteria.where("_id").regex(regex2));
//
//        List<MatrixDocument> results = mongoTemplate.find(finalQuery, MatrixDocument.class);
//
//        // 만약 C, T 중 하나만 존재하는 경우에 대한 처리
//        if (results.size() == 1) {
//            // 로그 추가 or 예외 처리 (C, T가 항상 있어야 하는데 1개만 존재할 경우 대비)
//            // System.out.println("경고: " + lastBatchCount + "의 C 또는 T 중 하나가 누락됨");
//        }
//        return results;
    }

    @Transactional
    public List<MatrixDocument> generateDocument(String inputPatchVersion, Integer batchCount) throws Exception {
        List<MatrixDocument> result = new ArrayList<>();
        String[] types = {"C","T"};
        // 모든 경우의 수의 ID 생성 => 공통적으로 사용 가능.
        List<Integer> classPool = poolManager.getClassPool();
        List<Integer> mapPool = poolManager.getMapPool();
        List<Integer> playerNumPool = poolManager.getPlayerNumPool();
        List<String> idList = generateMatrixId(mapPool,playerNumPool);
        // 클래스 별로 생성
        for(Integer classCode : classPool){
            for (int i = 0; i < types.length; i++) {
                // doc 하나 만들기 => id랑 type만 다르게 하면 됨.
                MatrixDocument doc = MatrixDocument.builder()
                        .id(String.join("/", inputPatchVersion,batchCount+"",types[i],classCode+""))
                        .batchCount(batchCount)
                        .type(types[i])
                        .cardPool(poolManager.getClassCardPoolMap().get(classCode))
                        .matrixMap(new HashMap<>())
                        .build();

                // 모든 ID + Matrix 스켈레톤을 matrixMap에 put.
                for (String id : idList) {
                    doc.getMatrixMap().put(id, generateSkeletonMatrix(classCode));
                }
                // 리턴 값에 저장.
                result.add(doc);
            }
        }

        return result;
    }

    // win 버전
    @Transactional
    public List<WinMatrixDocument> generateWinDocument(String inputPatchVersion, Integer batchCount) throws Exception {
        List<WinMatrixDocument> result = new ArrayList<>();
        String[] types = {"C","T"};
        // 모든 경우의 수의 ID 생성 => 공통적으로 사용 가능.
        List<Integer> classPool = poolManager.getClassPool();
        List<Integer> mapPool = poolManager.getMapPool();
        List<Integer> playerNumPool = poolManager.getPlayerNumPool();
        List<String> idList = generateMatrixId(mapPool,playerNumPool);
        // 클래스 별로 생성
        for(Integer classCode : classPool){
            for (int i = 0; i < types.length; i++) {
                // doc 하나 만들기 => id랑 type만 다르게 하면 됨.
                WinMatrixDocument doc = WinMatrixDocument.builder()
                        .id(String.join("/", inputPatchVersion,batchCount+"",types[i],classCode+""))
                        .type(types[i])
                        .cardPool(poolManager.getClassCardPoolMap().get(classCode))
                        .matrixMap(new HashMap<>())
                        .build();

                // 모든 ID + Matrix 스켈레톤을 matrixMap에 put.
                for (String id : idList) {
                    doc.getMatrixMap().put(id, generateSkeletonMatrix(classCode));
                }
                // 리턴 값에 저장.
                result.add(doc);
            }
        }

        return result;
    }

    // 재사용 할 Matrix 스켈레톤 생성. deepCopy가 없어서..
    public List<List<Integer>> generateSkeletonMatrix(int classCode) throws Exception {
        List<List<Integer>> skeletonMatrix = new ArrayList<>();
        List<Integer> row = new ArrayList<>();
        List<Integer> classCardPool = poolManager.getClassCardPoolMap().get(classCode);
        for (int col = 0; col < classCardPool.size(); col++) { // col 채우기
            row.add(0);
        }
        for (int i = 0; i < classCardPool.size()+1; i++) { // row 채우기
            skeletonMatrix.add(new ArrayList<>(row));
        }
        return skeletonMatrix;
    }



    // MatrixId 생성 로직 [완료]
    public List<String> generateMatrixId(List<Integer> mapPool,List<Integer> playerNumPool) throws Exception {
        List<String> result = new ArrayList<>();

        for(Integer mapID : mapPool){
            for(Integer playerNum : playerNumPool){
                String matrixId = String.join("/", mapID+"",playerNum+"");
                result.add(matrixId);
            }
        }

        return result;
    }

    // 전이 빈도 행렬 상에서 ++ 할 좌표 생성 [완료]
    public List<int[]> generateTransitionXY(PlayerLogRequestDto playerLog) {
        // 좌표를 저장할 리스트
        List<int[]> result = new ArrayList<>();

        // 재사용할 Random 인스턴스
        Random random = new Random();

        // 병합해서 관리할 List
        List<Integer> merged = new ArrayList<>();

        // 카드ID를 인덱스로 전환
        List<Integer> round1 = playerLog.getRound1Set();
        List<Integer> round2 = playerLog.getRound2Set();
        List<Integer> round3 = playerLog.getRound3Set();

//        Map<Integer, Integer> cardPoolIndexMap = poolManager.getCardPoolIndex();
//        Map<Integer, Integer> cardPoolIndexMap = poolManager.getCardPoolIndex();
        Map<Integer, Integer> cardPoolIndexMap = poolManager.getClassCardPoolIndexMap().get(playerLog.getClassCode());
        for (int i = 0; i < round1.size(); i++) {
            Integer cardId = round1.get(i);
            Integer cardIndex = cardPoolIndexMap.get(cardId);
            round1.set(i, cardIndex);
        }
        for (int i = 0; i < round2.size(); i++) {
            Integer cardId = round2.get(i);
            Integer cardIndex = cardPoolIndexMap.get(cardId);
            round2.set(i, cardIndex);
        }
        for (int i = 0; i < round3.size(); i++) {
            Integer cardId = round3.get(i);
            Integer cardIndex = cardPoolIndexMap.get(cardId);
            round3.set(i, cardIndex);
        }
        // 트랜지션 체크 시작
        // =============================================================================
        // round 0 >> 1 트랜지션 체크 => 기존 값을 row로 이후 값을 col으로 하는 좌표로
        for (int i = 0; i < round1.size(); i++) {
            result.add(new int[] {cardPoolIndexMap.size(), round1.get(i)});
        }
        // round 0 >> 1 트랜지션 랜덤 체크 => 이중 for문 조합 생성
        for (int i = 0; i < round1.size(); i++) {
            for (int j = i+1; j < round1.size(); j++) {
                int index1 = round1.get(i);
                int index2 = round1.get(j);
                if(random.nextBoolean()){
                    result.add(new int[] {index1, index2});
                    result.add(new int[] {index1, index1}); // 총계 계산용.
                } else{
                    result.add(new int[] {index2, index1});
                    result.add(new int[] {index2, index2});
                }
            }
        }
        merged.addAll(round1);
        // =============================================================================
        // round 1 >> 2 트랜지션 체크 => 기존 값을 row로 이후 값을 col으로 하는 좌표로
        for (int old = 0; old < merged.size(); old++) {
            for (int i = 0; i < round2.size(); i++) {
                result.add(new int[] {merged.get(old), round2.get(i)});
                result.add(new int[] {merged.get(old), merged.get(old)});
            }
        }
        // round 1 >> 2 트랜지션 랜덤 체크 => 이중 for문 조합 생성
        for (int i = 0; i < round2.size(); i++) {
            for (int j = i+1; j < round2.size(); j++) {
                int index1 = round2.get(i);
                int index2 = round2.get(j);
                if(random.nextBoolean()){
                    result.add(new int[] {index1, index2});
                    result.add(new int[] {index1, index1}); // 총계 계산용.
                } else{
                    result.add(new int[] {index2, index1});
                    result.add(new int[] {index2, index2});
                }
            }
        }
        merged.addAll(round2);
        // =============================================================================
        // round 2 >> 3 트랜지션 체크 => 기존 값을 row로 이후 값을 col으로 하는 좌표로
        for (int old = 0; old < merged.size(); old++) {
            for (int i = 0; i < round3.size(); i++) {
                result.add(new int[] {merged.get(old), round3.get(i)});
                result.add(new int[] {merged.get(old), merged.get(old)});
            }
        }
        // round 2 >> 3 트랜지션 랜덤 체크 => 이중 for문 조합 생성
        for (int i = 0; i < round3.size(); i++) {
            for (int j = i+1; j < round3.size(); j++) {
                int index1 = round3.get(i);
                int index2 = round3.get(j);
                if(random.nextBoolean()){
                    result.add(new int[] {index1, index2});
                    result.add(new int[] {index1, index1}); // 총계 계산용.
                } else{
                    result.add(new int[] {index2, index1});
                    result.add(new int[] {index2, index2});
                }
            }
        }
        merged.addAll(round3);

        // 생성 종료.
        return result;
    }
    // 공존 빈도 행렬 상에서 ++ 할 좌표 생성 [완료]
    public List<int[]> generateCoexistenceXY(PlayerLogRequestDto playerLog) {
        // 좌표를 저장할 리스트
        List<int[]> result = new ArrayList<>();

        // 카드셋 병합
        List<Integer> merged = new ArrayList<>();
        merged.addAll(playerLog.getRound1Set());
        merged.addAll(playerLog.getRound2Set());
        merged.addAll(playerLog.getRound3Set());
        // 카드 ID 를 인덱스로 전환
//        Map<Integer, Integer> cardPoolIndexMap = poolManager.getCardPoolIndex();
        Map<Integer, Integer> cardPoolIndexMap = poolManager.getClassCardPoolIndexMap().get(playerLog.getClassCode());
        for (int i = 0; i < merged.size(); i++) {
            Integer cardId = merged.get(i);
            Integer cardIndex = cardPoolIndexMap.get(cardId);
            merged.set(i, cardIndex);
        }
        // 행렬상의 row, col 좌표 생성
        for (int row = 0; row < merged.size(); row++) {
            for (int col = 0; col < merged.size(); col++) {
                result.add(new int[] {merged.get(row),merged.get(col)});
            }
        }

        return result;
    }

    // private 써도 되겠지? 이 서비스에서만 쓸건데 [완료]
    public void updateMatrix(MatrixDocument doc, List<String> targetMatrixIdList, List<int[]> xyList) {
        for (String id : targetMatrixIdList){
            List<List<Integer>> matrix= doc.getMatrixMap().get(id);
            for(int[] xy:xyList){
                int row = xy[0];
                int col = xy[1];
                int oldValue = matrix.get(row).get(col);
                matrix.get(row).set(col, oldValue+1);
            }
        }
    }

    public void updateWinMatrix(WinMatrixDocument doc, List<String> targetMatrixIdList, List<int[]> xyList) {
        for (String id : targetMatrixIdList){
            List<List<Integer>> matrix= doc.getMatrixMap().get(id);
            for(int[] xy:xyList){
                int row = xy[0];
                int col = xy[1];
                int oldValue = matrix.get(row).get(col);
                matrix.get(row).set(col, oldValue+1);
            }
        }
    }

    public void weightUpdateMatrix(MatrixDocument doc, List<String> targetMatrixIdList) {
        // 재료 load
        Map<Integer, Integer> upgradeCardPoolMap = poolManager.getUpgradeCardPoolMap();

        String[] temp = doc.getId().split("/");
        String classCodeStr = temp[3];
        Integer classCode = Integer.parseInt(classCodeStr);
        Map<Integer, Map<Integer, Integer>> classCardPoolIndexMap = poolManager.getClassCardPoolIndexMap();
        Map<Integer, Integer> classCardPoolIndex = classCardPoolIndexMap.get(classCode);

        // 타겟 matrix 순회
        for (String id : targetMatrixIdList){
            List<List<Integer>> matrix= doc.getMatrixMap().get(id);

            // 승급 강화 카드 순회하기.
            for (Map.Entry<Integer, Integer> entry : upgradeCardPoolMap.entrySet()){
                Integer upgradeCardId = entry.getValue();
                Integer upgradeReinforceCardId = entry.getKey();
                // 자기 직업 승급 강화 카드가 아니면 작업하면 안됨.
                if(classCardPoolIndex.containsKey(upgradeReinforceCardId)==false){
                    continue;
                }
                // Id => index 변환
                Integer upgradeCardIndex = classCardPoolIndex.get(upgradeCardId);
                Integer upgradeReinforceCardIndex = classCardPoolIndex.get(upgradeReinforceCardId);

                // matrix col의 길이만큼 순회하면서, 열 복사 => 열 고정, 행 순회
                for (int row = 0; row < matrix.get(0).size(); row++) {
                    // 교점 건너뛰기.
                    if(row==upgradeCardIndex || row==upgradeReinforceCardIndex){
                        continue;
                    }

                    // 승급 카드의 빈도 = parent, 승급 강화카드의 빈도는 child
                    int parent = matrix.get(row).get(upgradeCardIndex) *3 /10;
                    int child = matrix.get(row).get(upgradeReinforceCardIndex);
                    matrix.get(row).set(upgradeReinforceCardIndex, parent+child);
                }
//                // matrix col의 길이만큼 순회하면서, 행 복사 => 행 고정, 열 순회
//                for (int col = 0; col < matrix.get(0).size(); col++ㄷ) {
//                    // 교점 건너뛰기.
//                    if(col==upgradeCardIndex || col==upgradeReinforceCardIndex){
//                        continue;
//                    }
//
//                    // 승급 카드의 빈도 = parent, 승급 강화카드의 빈도는 child
//                    int parent = matrix.get(upgradeCardIndex).get(col);
//                    int child = matrix.get(upgradeReinforceCardIndex).get(col);
//                    matrix.get(upgradeReinforceCardIndex).set(col, parent+child);
//                }
//                // 건너뛴 교점 작업하기
//                int parent = matrix.get(upgradeCardIndex).get(upgradeCardIndex);
//                int child1 = matrix.get(upgradeCardIndex).get(upgradeReinforceCardIndex);
//                matrix.get(upgradeCardIndex).set(upgradeReinforceCardIndex, parent+child1);
//                int child2 = matrix.get(upgradeReinforceCardIndex).get(upgradeReinforceCardIndex);
//                matrix.get(upgradeReinforceCardIndex).set(upgradeReinforceCardIndex, parent+child2);

            }
        }
    }

}
