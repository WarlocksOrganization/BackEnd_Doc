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


}
