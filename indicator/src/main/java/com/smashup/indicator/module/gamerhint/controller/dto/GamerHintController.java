package com.smashup.indicator.module.gamerhint.controller.dto;

import com.smashup.indicator.common.util.AbstractRestController;
import com.smashup.indicator.module.gamerhint.controller.dto.request.GameEndRequestDto;
import com.smashup.indicator.module.gamerhint.controller.dto.request.InsertDataListRequestDto;
import com.smashup.indicator.module.gamerhint.controller.dto.request.InsertDataRequestDto;
import com.smashup.indicator.module.gamerhint.domain.entity.MatrixDocument;
import com.smashup.indicator.module.gamerhint.service.impl.GamerHintMatrixService;
import com.smashup.indicator.module.gamerhint.service.impl.GamerHintService;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.util.List;
import java.util.Map;

@RestController
@RequestMapping("/internal/hints")
@RequiredArgsConstructor

@Slf4j
public class GamerHintController extends AbstractRestController {

    // 의존성 주입
    private final GamerHintService gamerHintService;
    private final GamerHintMatrixService gamerHintMatrixService;

    // 데이터 수집 => API 테스트 필요
    // 개인적으로는 의미상 Post 보다는 Put이 더 가까우므로 PutMapping 하려고 했으나.
    // 아예 없을때는 update 보다는 insert가 되기도 하고.
    // 로그서버 맡은 팀원이 Post로 받아달라고 요청하였음.
    @PostMapping("/data")
    public ResponseEntity<Map<String, Object>> insertData(
            @RequestBody GameEndRequestDto dto
    ) throws Exception {
        try {
//            log.debug("insertData: {}", dto);
//            System.out.println(dto.toString());
            List<MatrixDocument> result = gamerHintMatrixService.insertData(dto);
            return handleSuccess(result); //
        } catch (Exception e) {
            return handleError(e.getMessage());
        }
    }

    @PostMapping("/test")
    public ResponseEntity<Map<String, Object>> dbtest() throws Exception {
        try {
            gamerHintMatrixService.dbtest();
            return handleSuccess("success!"); //
        } catch (Exception e) {
            return handleError(e.getMessage());
        }
    }

//    // 데이터 수집 => API 테스트 필요
//    @PostMapping("/data")
//    public ResponseEntity<Map<String, Object>> insertData(
//            @RequestBody InsertDataListRequestDto dto
//    ) throws Exception {
//        try {
////            log.debug("insertData: {}", dto);
//            gamerHintService.insertData(dto);
//            return handleSuccess(dto);
//        } catch (Exception e) {
//            return handleError(e.getMessage());
//        }
//    }

}
