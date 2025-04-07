package com.smashup.indicator.module.version.controller.dto;

import com.smashup.indicator.common.util.AbstractRestController;
import com.smashup.indicator.module.version.controller.dto.request.UpdatePoolRequestDto;
import com.smashup.indicator.module.version.controller.dto.request.UpdatePatchVersionRequestDto;
import com.smashup.indicator.module.version.service.impl.VersionService;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.util.Map;

@RestController
@RequestMapping("/api/v1/versions")
@RequiredArgsConstructor

@Slf4j
public class VersionController extends AbstractRestController {

    // 의존성 주입
    private final VersionService versionService;


    // 풀 업데이트 => 밸런스 패치 버전 수정 => API 테스트 성공
    @PostMapping("")
    public ResponseEntity<Map<String, Object>> updatePatchVersion(
            @RequestBody UpdatePoolRequestDto dto
    ) throws Exception {
        try {
            log.debug("updatePatchVersion: {}", dto);
            UpdatePoolRequestDto result = versionService.updatePatchVersion(dto);
            return handleSuccess(result);
        } catch (Exception e) {
            return handleError(e.getMessage());
        }
    }

    // 풀 업데이트 => API 테스트 성공
    @PostMapping("/pool")
    public ResponseEntity<Map<String, Object>> updatePool(
            @RequestBody UpdatePoolRequestDto dto
    ) throws Exception {
        try {
            log.debug("updateCardPool: {}", dto);
            UpdatePoolRequestDto result = versionService.updatePool(dto);
            return handleSuccess(result);
        } catch (Exception e) {
            return handleError(e.getMessage());
        }
    }

//    // 테스트용 배치카운트 변경 => API 테스트 성공 => 이제 무쓸모임. 배치카운트 올리고 싶으면 스케줄러 시간 조절해서 올리기
//    @PostMapping("/batch")
//    public ResponseEntity<Map<String, Object>> batchPlus() throws Exception {
//        try {
//            versionService.incrementBatchCount();
//            return handleSuccess(versionService.getBatchCount());
//        } catch (Exception e) {
//            return handleError(e.getMessage());
//        }
//    }

}
