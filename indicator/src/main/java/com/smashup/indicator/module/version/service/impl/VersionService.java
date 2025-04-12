package com.smashup.indicator.module.version.service.impl;

import com.smashup.indicator.module.gamerhint.domain.entity.MatrixDocument;
import com.smashup.indicator.module.gamerhint.domain.entity.WinMatrixDocument;
import com.smashup.indicator.module.gamerhint.repository.MatrixRepository;
import com.smashup.indicator.module.gamerhint.repository.WinMatrixRepository;
import com.smashup.indicator.module.gamerhint.service.impl.GamerHintMatrixSubService;
import com.smashup.indicator.module.version.PoolManager;
import com.smashup.indicator.module.version.controller.dto.request.UpdatePoolRequestDto;
import com.smashup.indicator.module.version.domain.entity.VersionWithPoolDocument;
import com.smashup.indicator.module.version.repository.VersionWithPoolRepository;
import lombok.Getter;
import lombok.RequiredArgsConstructor;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.util.List;

@RequiredArgsConstructor
@Service
@Getter
public class VersionService {
    // 의존성 주입
    private final PoolManager poolManager;
    private final GamerHintMatrixSubService gamerHintMatrixSubService;
    private final MatrixRepository matrixRepository;
    private final WinMatrixRepository winMatrixRepository;
    private final VersionWithPoolRepository versionWithPoolRepository;

    // 필드 변수
    private String currentPatchVersion = "not yet set";  // 기본 값 // 중앙 관리
    private int batchCount = 1;

    @Transactional
    public UpdatePoolRequestDto updatePatchVersion(UpdatePoolRequestDto dto) throws Exception {
        String newVersion = dto.getPatchVersion();
        // 같은 버전으로 바꿀때는 작동안하게.
        if(this.currentPatchVersion.equals(newVersion)){
            return null;
        }

        // 버전 업으로 인한 변경사항 세팅.
        // 관례적으로 cardPool 세팅이 먼저.
        UpdatePoolRequestDto result = updatePool(dto);
        this.currentPatchVersion = newVersion;


        // 서버 재실행하고 패치버전 세팅할때. 기존의 batchCount값 찾아서 세팅.
        try{
            // 해당 patchVersion의 document있으면 가장 마지막 batchCount로 세팅해주고
                // batchCount+1로 세팅해주면 batchCount+1의 document가 안 만들어져서. 연결이 안돼서 사고남.
            // 아래 최초 스켈레톤 안 만들도록 건너뛰기.
            if(gamerHintMatrixSubService.haveDocument(currentPatchVersion)){
                this.batchCount = gamerHintMatrixSubService.getLatestBatchCount(currentPatchVersion);
                return result;
            }

        } catch (Exception e) {
            e.printStackTrace();
        }

        // 카드 풀 및 배치 카운트 업데이트
        // poolManager.updateCardPool();
        resetBatchCount();

        // 배치 리셋이후 해당 패치버전의 첫 document 생성 및 저장 필요.
        try{
            List<MatrixDocument> docs = gamerHintMatrixSubService.generateDocument(currentPatchVersion, batchCount);
            for (MatrixDocument doc : docs) {
                matrixRepository.save(doc);
            }
            // win 버전
            List<WinMatrixDocument> winDocs = gamerHintMatrixSubService.generateWinDocument(currentPatchVersion, batchCount);
            for (WinMatrixDocument winDoc : winDocs) {
                winMatrixRepository.save(winDoc);
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        return result;
    }

//    @Transactional
    public UpdatePoolRequestDto updatePool(UpdatePoolRequestDto dto) {
        poolManager.updatePoolPost(dto);

        UpdatePoolRequestDto returnDto = UpdatePoolRequestDto.builder()
                .cardPoolMap(poolManager.getClassCardPoolMap())
                .classPool(poolManager.getClassPool())
                .mapPool(poolManager.getMapPool())
                .playerNumPool(poolManager.getPlayerNumPool())
                .patchVersion(poolManager.getPatchVersion())
                .allCardPoolMap(poolManager.getAllCardPoolMap())
                .build();
        return returnDto;
    }

    public void saveVersionWithPool(UpdatePoolRequestDto dto)  throws Exception {
        VersionWithPoolDocument doc = VersionWithPoolDocument.builder()
                .id(dto.getPatchVersion())
                .patchVersionNum(Integer.parseInt( dto.getPatchVersion() ) )
                .cardPoolMap(dto.getCardPoolMap())
                .classPool(dto.getClassPool())
                .mapPool(dto.getMapPool())
                .playerNumPool(dto.getPlayerNumPool())
                .allCardPoolMap(dto.getAllCardPoolMap())
                .build();
        versionWithPoolRepository.save(doc);
    }

    // 이전 BatchCountManager 로직
    public synchronized void resetBatchCount() {
        this.batchCount = 1; // PATCH_VERSION이 바뀌면 초기화
    }

    public synchronized void incrementBatchCount() {
        this.batchCount++; // PATCH_VERSION이 바뀌면 초기화
    }

}
