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
import lombok.extern.slf4j.Slf4j;
import org.springframework.dao.OptimisticLockingFailureException;

import org.springframework.retry.annotation.Backoff;
import org.springframework.retry.annotation.Retryable;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.util.*;

@Service
@RequiredArgsConstructor
@Slf4j
public class GamerHintMatrixService {
    // 의존성 주입
    private final GamerHintMatrixSubService gamerHintMatrixSubService;
    private final MatrixRepository matrixRepository;
    private final WinMatrixRepository winMatrixRepository;
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
            String matrixId = "-1/-1";
            extractDoc.getMatrixMap().put(matrixId, doc.getMatrixMap().get(matrixId));

            results.add(extractDoc);
        }
        return results;
    }
    // 지표 전달하기.
    @Transactional
    public List<MatrixDocument> getIndicatorAllPick() throws Exception {
        // 콜드스타트 문구 발송
        List<MatrixDocument> temp = gamerHintMatrixSubService.getDocumentByBatch(versionService.getCurrentPatchVersion(),versionService.getBatchCount());
        return temp;
    }
    // 지표 전달하기.
    @Transactional
    public List<WinMatrixDocument> getIndicatorAllWin() throws Exception {
        // 콜드스타트 문구 발송
        List<WinMatrixDocument> temp = gamerHintMatrixSubService.getWinDocumentByBatch(versionService.getCurrentPatchVersion(),versionService.getBatchCount());
        return temp;
    }

    // 데이터 수집
    @Retryable(
            value = { OptimisticLockingFailureException.class },
            maxAttempts = 3,
            backoff = @Backoff(delay = 2000, multiplier = 2)
    )
    @Transactional
    public List<MatrixDocument> insertData(LogServerRequestDto dto) throws Exception {
        // 도큐먼트 세팅
        List<MatrixDocument> docs = gamerHintMatrixSubService.getDocumentByBatch(versionService.getCurrentPatchVersion(), versionService.getBatchCount());
        List<WinMatrixDocument> winDocs = gamerHintMatrixSubService.getWinDocumentByBatch(versionService.getCurrentPatchVersion(), versionService.getBatchCount());

        // 여러 게임 받도록 설계 변경.
        for (GameEndRequestDto game : dto.getData()) {
            try {
                // 이상한 버전의 게임은 처리X => early return => 밸런스 패치 버전만 확인.
                String[] patchParts = game.getPatchVersion().split("\\.");
                if (patchParts.length < 3) {
                    // 로그로 찍어두는 것도 좋고, 이 게임을 건너뛰는 것도 가능
                    log.warn("Invalid patch version format: {}", game.getPatchVersion());
                    continue;
                }
                String balancePatchVersion = patchParts[2];

                if (balancePatchVersion.equals(versionService.getCurrentPatchVersion()) == false) {
                    continue;
                }
                // 우승자 찾기
                int winnerScore = 0;
                for (PlayerLogRequestDto playerLog : game.getPlayerLogs()) {
                    List<Integer> roundScores = playerLog.getRoundScore();
                    int scoreSum = 0;
                    for (int score : roundScores) {
                        scoreSum += score;
                    }
                    winnerScore = Math.max(winnerScore, scoreSum);
                }

                // playerLog 단위 작업
                for (PlayerLogRequestDto playerLog : game.getPlayerLogs()) {
                    // classCode : 100인 버그 로그 패스
                    if (playerLog.getClassCode() == 100) {
                        continue;
                    }

                    // targetMatrixId 생성
                    /// pool 세팅
    //                List<Integer> classPool = List.of(playerLog.getClassCode(),-1);
                    List<Integer> mapPool = List.of(game.getMapId(), -1);
                    List<Integer> playerNumPool = List.of(game.getPlayerCount(), -1);

                    List<String> targetMatrixIdList = gamerHintMatrixSubService.generateMatrixId(mapPool, playerNumPool);

                    // 공존 빈도 행렬 상에서 ++ 할 좌표 생성 => 공존 빈도 행렬 docC에 id와 좌표에 맞게 반영
                    List<int[]> coexistenceXYList = gamerHintMatrixSubService.generateCoexistenceXY(playerLog);
                    // 전이 빈도 행렬 상에서 ++ 할 좌표 생성 => 전이 빈도 행렬 docT에 id와 좌표에 맞게 반영
                    List<int[]> transitionXYList = gamerHintMatrixSubService.generateTransitionXY(playerLog);

                    // 픽률만
                    for (MatrixDocument doc : docs) {
                        // 다른 직업은 패싱
                        String docClass = doc.getId().split("/")[3];
                        if (docClass.equals(playerLog.getClassCode() + "") == false) {
                            continue;
                        }
                        // C | T 구분
                        if (doc.getType().equals("C")) {
                            gamerHintMatrixSubService.updateMatrix(doc, targetMatrixIdList, coexistenceXYList);
                        } else {
                            gamerHintMatrixSubService.updateMatrix(doc, targetMatrixIdList, transitionXYList);
                        }
                    }
                    // 우승 찾기
                    List<Integer> roundScores = playerLog.getRoundScore();
                    int scoreSum = 0;
                    for (int score : roundScores) {
                        scoreSum += score;
                    }
                    if (scoreSum == winnerScore) {
                        for (WinMatrixDocument winDoc : winDocs) {
                            // 다른 직업은 패싱
                            String docClass = winDoc.getId().split("/")[3];
                            if (docClass.equals(playerLog.getClassCode() + "") == false) {
                                continue;
                            }
                            // C | T 구분
                            if (winDoc.getType().equals("C")) {
                                gamerHintMatrixSubService.updateWinMatrix(winDoc, targetMatrixIdList, coexistenceXYList);
                            } else {
                                gamerHintMatrixSubService.updateWinMatrix(winDoc, targetMatrixIdList, transitionXYList);
                            }
                        }
                    }
                }

            } catch (Exception e){
                log.error("Exception while processing gameId {}: {}", game.getGameId(), e.getMessage(), e);
            }
    }

        // 새로 들어온 data 전부 반영된 상태이므로 저장.
        for (MatrixDocument doc : docs) {
            matrixRepository.save(doc);
        }
        for (WinMatrixDocument winDoc : winDocs) {
            winMatrixRepository.save(winDoc);
        }

        return docs;
    }

}
