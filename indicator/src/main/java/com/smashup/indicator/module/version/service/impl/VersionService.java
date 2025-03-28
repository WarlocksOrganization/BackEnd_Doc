package com.smashup.indicator.module.version.service.impl;

import com.smashup.indicator.module.version.BatchCountManager;
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
    private final BatchCountManager batchCountManager;
    // 필드 변수
    private String oldPatchVersion = null;  // 기본 값 // 중앙 관리
    private String currentPatchVersion = "1.0.0";  // 기본 값 // 중앙 관리

//    @Transactional
    public String updatePatchVersion(UpdatePatchVersionRequestDto dto) {
        // 임시방편으로 비밀번호 틀리면 작동안하게.
        // 인증인가가 없어서.
        // timeStamp라고 이름 속이기.
        // Dto 납치되고 분석당하면 의미없음.
        if(dto.getTimeStamp().equals("20250320")==false){
            return null;
        }

        String newVersion = dto.getNewPatchVersion();
        if (!this.currentPatchVersion.equals(newVersion)) {  // 실제 변경될 때만 실행
            this.oldPatchVersion = currentPatchVersion;
            this.currentPatchVersion = newVersion;
            // 서버 내려가고 다시 초기화해줄때,요청에서 직전 버전까지 주입해주기.
            if(dto.getOldPatchVersion()!=null){
                this.oldPatchVersion = dto.getOldPatchVersion();
            }

            // 카드 풀 및 배치 카운트 업데이트
//            poolManager.updateCardPool();
            batchCountManager.resetBatchCount();
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
                .build();
        return returnDto;
    }

}
