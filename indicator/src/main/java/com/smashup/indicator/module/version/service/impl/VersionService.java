package com.smashup.indicator.module.version.service.impl;

import com.smashup.indicator.module.gamerhint.domain.entity.MatrixDocument;
import com.smashup.indicator.module.gamerhint.domain.entity.WinMatrixDocument;
import com.smashup.indicator.module.gamerhint.repository.MatrixRepository;
import com.smashup.indicator.module.gamerhint.repository.WinMatrixRepository;
import com.smashup.indicator.module.gamerhint.service.impl.GamerHintMatrixSubService;
import com.smashup.indicator.module.version.PoolManager;
import com.smashup.indicator.module.version.controller.dto.request.UpdatePatchVersionRequestDto;
import com.smashup.indicator.module.version.controller.dto.request.UpdatePoolRequestDto;
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

    // 필드 변수
    private String oldPatchVersion = "not yet set";  // 기본 값 // 중앙 관리
    private String currentPatchVersion = "not yet set";  // 기본 값 // 중앙 관리
    private int batchCount = 1;

    @Transactional
    public String updatePatchVersion(UpdatePatchVersionRequestDto dto) throws Exception {
        // 임시방편으로 비밀번호 틀리면 작동안하게.
        // 인증인가가 없어서.
        // timeStamp라고 이름 속이기.
        // Dto 납치되고 분석당하면 의미없음.
        if(dto.getTimeStamp().equals("20250320")==false){
            return null;
        }

        String newVersion = dto.getNewPatchVersion();
        if (!this.currentPatchVersion.equals(newVersion)) {  // 실제 변경될 때만 실행
            if( ! poolManager.getPatchVersion().equals(newVersion)){    // newVersion값과 poolManager가 가진 값이 다르면 예외처리
                throw new Exception("poolManger 세팅 먼저.");
            }
            this.oldPatchVersion = currentPatchVersion;
            this.currentPatchVersion = newVersion;
            // 서버 내려가고 다시 초기화해줄때,요청에서 직전 버전까지 주입해주기.
            if(dto.getOldPatchVersion()!=null){
                this.oldPatchVersion = dto.getOldPatchVersion();
            }
            // 서버 내려가고 다시 초기화해줄때 진행된 batchCount가 있을때.
            if(dto.getLatestBatchCount() != null){
                // 이전 버전batchCount를 한번에 set하기
                this.batchCount = dto.getLatestBatchCount();
                // 해당 patchVersion 로 작성된 문서가 이미 있으면 최초 스켈레톤 안 만들도록 건너뛰기.
                try{
                    // 있으면 아래 로직 건너뜀.
                    if(gamerHintMatrixSubService.haveDocument(currentPatchVersion)){
                        return currentPatchVersion;
                    }

                } catch (Exception e) {
                    e.printStackTrace();
                }

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
        }
        return currentPatchVersion;
    }

//    @Transactional
    public UpdatePoolRequestDto updatePool(UpdatePoolRequestDto dto) {
        // 임시방편으로 비밀번호 틀리면 작동안하게.
        // 인증인가가 없어서.
        // timeStamp라고 이름 속이기.
        // Dto 납치되고 분석당하면 의미없음.
        if(dto.getTimeStamp().equals("20250320")==false){
            return null;
        }
        poolManager.updatePoolPost(dto);

        UpdatePoolRequestDto returnDto = UpdatePoolRequestDto.builder()
                .cardPool(poolManager.getCardPool())
                .classPool(poolManager.getClassPool())
                .mapPool(poolManager.getMapPool())
                .playerNumPool(poolManager.getPlayerNumPool())
                .patchVersion(poolManager.getPatchVersion())
                .build();
        return returnDto;
    }
    // 이전 BatchCountManager 로직
    public synchronized void resetBatchCount() {
        this.batchCount = 1; // PATCH_VERSION이 바뀌면 초기화
    }

    public synchronized void incrementBatchCount() {
        this.batchCount++; // PATCH_VERSION이 바뀌면 초기화
    }

}
