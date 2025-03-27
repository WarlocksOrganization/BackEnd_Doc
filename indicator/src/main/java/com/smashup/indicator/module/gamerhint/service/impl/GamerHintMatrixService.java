package com.smashup.indicator.module.gamerhint.service.impl;

import com.smashup.indicator.module.gamerhint.controller.dto.request.GameEndRequestDto;
import com.smashup.indicator.module.gamerhint.controller.dto.request.PlayerLogRequestDto;
import com.smashup.indicator.module.gamerhint.domain.entity.MatrixDocument;
import com.smashup.indicator.module.gamerhint.repository.MatrixRepository;
import com.smashup.indicator.module.version.BatchCountManager;
import com.smashup.indicator.module.version.PoolManager;
import com.smashup.indicator.module.version.service.impl.VersionService;
import lombok.RequiredArgsConstructor;
import org.bson.types.ObjectId;
import org.springframework.data.domain.Sort;
import org.springframework.data.mongodb.core.MongoTemplate;
import org.springframework.data.mongodb.core.query.*;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.util.*;

@Service
@RequiredArgsConstructor
public class GamerHintMatrixService {
    // 의존성 주입
    private final MatrixRepository matrixRepository;
    private final PoolManager poolManager;
    private final BatchCountManager batchCountManager;
    private final VersionService versionService;
    private final MongoTemplate mongoTemplate;





    // 데이터 수집
    @Transactional
    public List<MatrixDocument> insertData(GameEndRequestDto dto) throws Exception {

        // 도큐먼트 세팅
        List<MatrixDocument> docs = setDocument(dto.getPatchVersion());
        if(docs == null){ // 설계상 게임 단위로 받는중. 이상한 버전의 게임이라 작동 X
            return null;
        }

        // playerLog 단위 작업
        for ( PlayerLogRequestDto playerLog: dto.getPlayerLogs()) {

            // targetMatrixId 생성
            /// pool 세팅
            List<Integer> classPool = List.of(playerLog.getClassCode(),-1);
            List<Integer> mapPool = List.of(dto.getMapId(),-1);
            List<Integer> playerNumPool = List.of(dto.getPlayerCount(),-1);

            List<String> targetMatrixIdList = generateMatrixId(classPool,mapPool,playerNumPool);

            // 공존 빈도 행렬 상에서 ++ 할 좌표 생성 => 공존 빈도 행렬 docC에 id와 좌표에 맞게 반영
            List<int[]> coexistenceXYList = generateCoexistenceXY(playerLog);
            // 전이 빈도 행렬 상에서 ++ 할 좌표 생성 => 전이 빈도 행렬 docT에 id와 좌표에 맞게 반영
            List<int[]> transitionXYList = generateTransitionXY(playerLog);
            for (MatrixDocument doc : docs) {
                if(doc.getType().equals("C")){
                    updateMatrix(doc, targetMatrixIdList, coexistenceXYList);
                } else{
                    updateMatrix(doc, targetMatrixIdList, transitionXYList);
                }
            }

        }

        // 새로 들어온 data 전부 반영된 상태이므로 저장.
        for (MatrixDocument doc : docs) {
            matrixRepository.save(doc);
        }
        System.out.println(docs);

        return docs;
    }



    // 도큐먼트 세팅 | 공통 컴포넌트1 참고 [미완료]
    // save만 하면 알아서 잘 들어가게 id 세팅은 여기서 해결.
    public List<MatrixDocument> setDocument(String inputPatchVersion) throws Exception {
        // 폐기
//        // PatchVersion 분류 => 직전 버전, 현재 버전
//        /// 직전 버전
//        if(inputPatchVersion.equals(versionService.getOldPatchVersion()) ){
//            // 마지막 배치 load
//            return getLatestDocument(inputPatchVersion);
//        }
        /// 현재 버전
        if (inputPatchVersion.equals(versionService.getCurrentPatchVersion())) {
            // 현재 배치 load 결과가 null
                // 현재 배치가 1이면 스켈레톤 새로 만들기.
                    // => 아무런 데이터 삽입 없이 배치카운트가 올라가면 작동 안하네.
                    // getLatestDocument() 해서 null이면 스켈레톤 새로 만드는걸로 바꾸자.
                // 현재 배치가 1이 아니면 직전 배치 가져와서 ID값을 현재 배치로 수정하기. [완료]
            // 현재 배치 load 결과가 null이 아니면 그냥 그대로 사용. [완료]
            List<MatrixDocument> docs = getDocumentByBatch(inputPatchVersion, batchCountManager.getBatchCount());
            if (docs.isEmpty()) {
                if (getLatestDocument(inputPatchVersion)==false) {
                    System.out.println("여긴가111111");

                    return generateDocument(inputPatchVersion);
                } else{ // 현재 배치가 1이 아니면 직전 배치 가져와서 ID값을 현재 배치로 수정하기. [완료]
                    System.out.println("여긴가222222");

                    docs = getDocumentByBatch(inputPatchVersion, batchCountManager.getBatchCount()-1);
                    for (MatrixDocument doc: docs) {
                        String id = doc.getId();
                        String newId = id.replace("/"+(batchCountManager.getBatchCount()-1)+"/","/"+(batchCountManager.getBatchCount())+"/");
                        doc.setId(newId);
                    }
                    return docs;
                }
            }
            else{ // 현재 배치 load 결과가 null이 아니면 그냥 그대로 사용. [완료]
                System.out.println("여긴가33333");
                System.out.println(docs);

                return docs;
            }
        }
        /// 취급 안하는 값. 버려야 함.
        else{
            System.out.println("여긴가44444");

            return null; // 임시 값
        }
    }

    // 현재 batchCount의 document 2개 가져오기.
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



    // 마지막 배치 (가장 최신 배치) load
    @Transactional
    public boolean getLatestDocument(String inputPatchVersion) throws Exception {
        // 1. 가장 마지막 batchCount 하나만 가져옴 => batchCount 안전하게 가져오기
        String regex1 = "^"+inputPatchVersion;
        Query batchCountCheckQuery = new Query();
        batchCountCheckQuery.addCriteria(Criteria.where("_id").regex(regex1));
        batchCountCheckQuery.with(Sort.by(Sort.Direction.DESC, "_id"));
        batchCountCheckQuery.limit(1);

        boolean result = mongoTemplate.exists(batchCountCheckQuery, MatrixDocument.class);
        return result;
        // 해당 패치 버전의 문서가 있냐 없냐로만 사용하기로 바뀜.
//        // 검색결과가 없음 => inputPatchVersion 에 해당되는 Document 가 없음.
//        if (lastBatchDoc == null) {
//            return null; // 임시값. => 예외처리 필요. 뭘 돌려주지???
//        }
//
//        // 2. 마지막 batchCount를 추출
//        String lastBatchCount = lastBatchDoc.getId().split("/")[1];
//
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

    public List<MatrixDocument> generateDocument(String inputPatchVersion) throws Exception {
        List<MatrixDocument> result = new ArrayList<>();
        String[] types = {"C","T"};
        // 모든 경우의 수의 ID 생성 => 공통적으로 사용 가능.
        List<Integer> classPool = poolManager.getClassPool();
        List<Integer> mapPool = poolManager.getMapPool();
        List<Integer> playerNumPool = poolManager.getPlayerNumPool();
        List<String> idList = generateMatrixId(classPool,mapPool,playerNumPool);
        for (int i = 0; i < types.length; i++) {
            // doc 하나 만들기 => id랑 type만 다르게 하면 됨.
            MatrixDocument doc = MatrixDocument.builder()
                    .id(String.join("/", inputPatchVersion,batchCountManager.getBatchCount()+"",types[i]))
                    .type(types[i])
                    .cardPool(poolManager.getCardPool())
                    .matrixMap(new HashMap<>())
                    .build();

            // 모든 ID + Matrix 스켈레톤을 matrixMap에 put.
            for (String id : idList) {
                doc.getMatrixMap().put(id, generateSkeletonMatrix());
            }
            // 리턴 값에 저장.
            result.add(doc);
        }

        return result;
    }

    // 재사용 할 Matrix 스켈레톤 생성. deepCopy가 없어서..
    public List<List<Integer>> generateSkeletonMatrix() throws Exception {
        List<List<Integer>> skeletonMatrix = new ArrayList<>();
        List<Integer> row = new ArrayList<>();
        for (int col = 0; col < poolManager.getCardPool().size(); col++) { // col 채우기
            row.add(0);
        }
        for (int i = 0; i < poolManager.getCardPool().size()+1; i++) { // row 채우기
            skeletonMatrix.add(new ArrayList<>(row));
        }
        return skeletonMatrix;
    }



    // MatrixId 생성 로직 [완료]
    public List<String> generateMatrixId(List<Integer> classPool, List<Integer> mapPool,List<Integer> playerNumPool) throws Exception {
        List<String> result = new ArrayList<>();
//        // pool 세팅
//        classPool.add(-1);
//        mapPool.add(-1);
//        playerNumPool.add(-1);
        //
        for(Integer classID : classPool){
            for(Integer mapID : mapPool){
                for(Integer playerNum : playerNumPool){
                    String matrixId = String.join("/", classID+"",mapID+"",playerNum+"");
                    result.add(matrixId);
                }
            }
        }
//        // BONUS 추가
//        result.add("-1/-1/-1");
        return result;
    }



    // 전이 빈도 행렬 상에서 ++ 할 좌표 생성 [완료]
    private List<int[]> generateTransitionXY(PlayerLogRequestDto playerLog) {
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

        Map<Integer, Integer> cardPoolIndexMap = poolManager.getCardPoolIndex();
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
    private List<int[]> generateCoexistenceXY(PlayerLogRequestDto playerLog) {
        // 좌표를 저장할 리스트
        List<int[]> result = new ArrayList<>();

        // 카드셋 병합
        List<Integer> merged = new ArrayList<>();
        merged.addAll(playerLog.getRound1Set());
        merged.addAll(playerLog.getRound2Set());
        merged.addAll(playerLog.getRound3Set());
        // 카드 ID 를 인덱스로 전환
        Map<Integer, Integer> cardPoolIndexMap = poolManager.getCardPoolIndex();
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
    private void updateMatrix(MatrixDocument docC, List<String> targetMatrixIdList, List<int[]> coexistenceXYList) {
        for (String id : targetMatrixIdList){
            List<List<Integer>> matrix= docC.getMatrixMap().get(id);
            for(int[] xy:coexistenceXYList){
                int row = xy[0];
                int col = xy[1];
                int oldValue = matrix.get(row).get(col);
                matrix.get(row).set(col, oldValue+1);
            }
        }
    }

    @Transactional
    public void dbtest() {
        MatrixDocument test = MatrixDocument.builder()
                .id(new ObjectId().toString())
                .type("test")
                .build();
        matrixRepository.save(test);
    }

}
