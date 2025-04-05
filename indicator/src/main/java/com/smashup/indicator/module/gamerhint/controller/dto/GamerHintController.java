package com.smashup.indicator.module.gamerhint.controller.dto;

import com.smashup.indicator.common.util.AbstractRestController;
import com.smashup.indicator.module.gamerhint.controller.dto.request.LogServerRequestDto;
import com.smashup.indicator.module.gamerhint.controller.dto.response.RebalanceResponseDto;
import com.smashup.indicator.module.gamerhint.domain.entity.MatrixDocument;
import com.smashup.indicator.module.gamerhint.domain.entity.WinMatrixDocument;
import com.smashup.indicator.module.gamerhint.service.impl.GamerHintMatrixService;
//import com.smashup.indicator.module.gamerhint.service.impl.GamerHintService;
import com.smashup.indicator.module.gamerhint.service.impl.MasterHintMatrixService;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.util.List;
import java.util.Map;

@RestController
@RequestMapping("")
@RequiredArgsConstructor

@Slf4j
public class GamerHintController extends AbstractRestController {

    // 의존성 주입
//    private final GamerHintService gamerHintService;
    private final GamerHintMatrixService gamerHintMatrixService;
    private final MasterHintMatrixService masterHintMatrixService;

    // 데이터 수집 => API 테스트 성공
    // 개인적으로는 의미상 Post 보다는 Put이 더 가까우므로 PutMapping 하려고 했으나.
    // 아예 없을때는 update 보다는 insert가 되기도 하고.
    // 로그서버 맡은 팀원이 Post로 받아달라고 요청하였음.
    @PostMapping("/internal/hints/data")
    public ResponseEntity<Map<String, Object>> insertData(
            @RequestBody LogServerRequestDto dto
    ) throws Exception {
        try {
            List<MatrixDocument> result = gamerHintMatrixService.insertData(dto);
            return handleSuccess(result); //
        } catch (Exception e) {
            return handleError(e.getMessage());
        }
    }

//    @PostMapping("/test")
//    public ResponseEntity<Map<String, Object>> dbtest() throws Exception {
//        try {
//            gamerHintMatrixService.dbtest();
//            return handleSuccess("success!"); //
//        } catch (Exception e) {
//            return handleError(e.getMessage());
//        }
//    }

    // 데이터 반출 => API 테스트 성공
    @GetMapping("/hints/pickrate")
    public ResponseEntity<Map<String, Object>> getIndicatorAllPick() throws Exception {
        try {
            log.debug("GetIndicator: {}");
            List<MatrixDocument> result = gamerHintMatrixService.getIndicatorAllPick();
            return handleSuccess(result);
        } catch (Exception e) {
            return handleError(e.getMessage());
        }
    }

    // 데이터 반출 => API 테스트 성공
    @GetMapping("/hints/winrate")
    public ResponseEntity<Map<String, Object>> getIndicatorAllWin() throws Exception {
        try {
            log.debug("GetIndicator: {}");
            List<WinMatrixDocument> result = gamerHintMatrixService.getIndicatorAllWin();
            return handleSuccess(result);
        } catch (Exception e) {
            return handleError(e.getMessage());
        }
    }

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

    // 데이터 수집 => API 테스트 성공
    @GetMapping("/hints")
    public ResponseEntity<Map<String, Object>> getIndicator() throws Exception {
        try {
            log.debug("GetIndicator: {}");
            List<MatrixDocument> result = gamerHintMatrixService.getIndicator();
            if(result==null){
                return handleSuccess("sorry, now cold start");
            } else{
                return handleSuccess(result);
            }
        } catch (Exception e) {
            return handleError(e.getMessage());
        }
    }

}
