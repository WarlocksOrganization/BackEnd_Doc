package com.smashup.indicator.module.masterhint.controller.dto;

import com.smashup.indicator.common.util.AbstractRestController;
import com.smashup.indicator.module.masterhint.controller.dto.response.RebalanceResponseDto;
import com.smashup.indicator.module.masterhint.service.impl.MasterHintMatrixCardService;
import com.smashup.indicator.module.masterhint.service.impl.MasterHintMatrixService;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.util.List;
import java.util.Map;

@RestController
@RequestMapping("/master")
@RequiredArgsConstructor

@Slf4j
public class MasterHintController extends AbstractRestController {

    // 의존성 주입
    private final MasterHintMatrixService masterHintMatrixService;
    private final MasterHintMatrixCardService masterHintMatrixCardService;


    // 데이터 반출 => API 테스트 성공
    @GetMapping("/hints/class")
    public ResponseEntity<Map<String, Object>> getClassPickWin() throws Exception {
        try {
            log.debug("getClassPickWin: {}");
            List<RebalanceResponseDto> result = masterHintMatrixService.generateRebalanceClass();
            return handleSuccess(result);
        } catch (Exception e) {
            return handleError(e.getMessage());
        }
    }

    // 데이터 반출 => API 테스트 성공
    @GetMapping("/hints/cards/{classCode}")
    public ResponseEntity<Map<String, Object>> getCardPickWin(
            @PathVariable int classCode
    ) throws Exception {
        try {
            log.debug("getCardPickWin: {}");
            List<RebalanceResponseDto> result = masterHintMatrixCardService.generateRebalanceCard(classCode);
            return handleSuccess(result);
        } catch (Exception e) {
            return handleError(e.getMessage());
        }
    }

}
