package com.smashup.indicator.module.gamerhint.service.impl;

import com.smashup.indicator.module.gamerhint.controller.dto.request.GameEndRequestDto;
import com.smashup.indicator.module.gamerhint.controller.dto.request.PlayerLogRequestDto;
import com.smashup.indicator.module.gamerhint.domain.entity.MatrixDocument;
import com.smashup.indicator.module.gamerhint.repository.MatrixRepository;
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
public class GamerHintMatrixService {
    // 의존성 주입
    private final GamerHintMatrixSubService gamerHintMatrixSubService;
    private final MatrixRepository matrixRepository;
    private final PoolManager poolManager;
    private final VersionService versionService;


    // 지표 전달하기.
    @Transactional
    public List<MatrixDocument> getIndicator() throws Exception {
        // 콜드스타트 문구 발송
        if(versionService.getBatchCount()==1){
            return null;
        }

        List<MatrixDocument> temp = gamerHintMatrixSubService.getDocumentByBatch(versionService.getCurrentPatchVersion(),versionService.getBatchCount()-1);
        List<MatrixDocument> results = new ArrayList<>();
        for (MatrixDocument doc: temp) {
            MatrixDocument extractDoc = MatrixDocument.builder()
                    .id(doc.getId())
                    .type(doc.getType())
                    .version(doc.getVersion())
                    .cardPool(doc.getCardPool())
                    .matrixMap(new HashMap<>())
                    .build();
            // 필요한 값만 extractDoc에 넣기.
            for( int classCode : poolManager.getClassPool()){
                if(classCode==-1)
                    continue;
                String matrixId = String.join("/",classCode+"","-1/-1");
                extractDoc.getMatrixMap().put(matrixId, doc.getMatrixMap().get(matrixId));
            }
            // extractDoc를 리턴 값에 저장
            results.add(extractDoc);
        }
        return results;
    }

    // 데이터 수집
    @Retryable(
            value = { OptimisticLockingFailureException.class },
            maxAttempts = 3,
            backoff = @Backoff(delay = 2000, multiplier = 2)
    )
    @Transactional
    public List<MatrixDocument> insertData(GameEndRequestDto dto) throws Exception {
        // 설계상 게임 단위로 받는중. 이상한 버전의 게임은 처리X => early return
        if (dto.getPatchVersion().equals(versionService.getCurrentPatchVersion()) == false) {
            return null;
        }
        // 도큐먼트 세팅
        List<MatrixDocument> docs = gamerHintMatrixSubService.getDocumentByBatch(versionService.getCurrentPatchVersion(), versionService.getBatchCount());

        // playerLog 단위 작업
        for ( PlayerLogRequestDto playerLog: dto.getPlayerLogs()) {

            // targetMatrixId 생성
            /// pool 세팅
            List<Integer> classPool = List.of(playerLog.getClassCode(),-1);
            List<Integer> mapPool = List.of(dto.getMapId(),-1);
            List<Integer> playerNumPool = List.of(dto.getPlayerCount(),-1);

            List<String> targetMatrixIdList = gamerHintMatrixSubService.generateMatrixId(classPool,mapPool,playerNumPool);

            // 공존 빈도 행렬 상에서 ++ 할 좌표 생성 => 공존 빈도 행렬 docC에 id와 좌표에 맞게 반영
            List<int[]> coexistenceXYList = gamerHintMatrixSubService.generateCoexistenceXY(playerLog);
            // 전이 빈도 행렬 상에서 ++ 할 좌표 생성 => 전이 빈도 행렬 docT에 id와 좌표에 맞게 반영
            List<int[]> transitionXYList = gamerHintMatrixSubService.generateTransitionXY(playerLog);
            for (MatrixDocument doc : docs) {
                if(doc.getType().equals("C")){
                    gamerHintMatrixSubService.updateMatrix(doc, targetMatrixIdList, coexistenceXYList);
                } else{
                    gamerHintMatrixSubService.updateMatrix(doc, targetMatrixIdList, transitionXYList);
                }
            }
        }

        // 새로 들어온 data 전부 반영된 상태이므로 저장.
        for (MatrixDocument doc : docs) {
            matrixRepository.save(doc);
        }

        return docs;
    }

}
