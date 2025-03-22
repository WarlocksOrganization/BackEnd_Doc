package com.smashup.indicator.module.gamerhint.controller.dto;

import com.smashup.indicator.common.util.AbstractRestController;
import com.smashup.indicator.module.gamerhint.controller.dto.request.InsertDataListRequestDto;
import com.smashup.indicator.module.gamerhint.controller.dto.request.InsertDataRequestDto;
import com.smashup.indicator.module.gamerhint.service.impl.GamerHintService;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.util.List;
import java.util.Map;

@RestController
@RequestMapping("/api/v1/hints")
@RequiredArgsConstructor

@Slf4j
public class GamerHintController extends AbstractRestController {

    // 의존성 주입
    private final GamerHintService gamerHintService;

    // 데이터 수집 => API 테스트 필요
    @PutMapping("/data")
    public ResponseEntity<Map<String, Object>> insertData(
            @RequestBody InsertDataListRequestDto dto
    ) throws Exception {
        try {
//            log.debug("insertData: {}", dto);
            gamerHintService.insertData(dto);
            return handleSuccess(dto);
        } catch (Exception e) {
            return handleError(e.getMessage());
        }
    }

}
