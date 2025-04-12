package com.smashup.indicator.module.gamerhint.controller.dto;

import com.smashup.indicator.common.util.AbstractRestController;
import com.smashup.indicator.module.gamerhint.controller.dto.request.LogServerRequestDto;
import com.smashup.indicator.module.gamerhint.domain.entity.MatrixDocument;
import com.smashup.indicator.module.gamerhint.domain.entity.WinMatrixDocument;
import com.smashup.indicator.module.gamerhint.service.impl.GamerHintMatrixService;
//import com.smashup.indicator.module.gamerhint.service.impl.GamerHintService;
import com.smashup.indicator.module.gamerhint.service.impl.GamerHintMatrixSubService;
import com.smashup.indicator.module.version.ReadyMadeManager;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

@RestController
@RequestMapping("")
@RequiredArgsConstructor

@Slf4j
public class GamerHintController extends AbstractRestController {

    // 의존성 주입
    private final GamerHintMatrixService gamerHintMatrixService;
    private final GamerHintMatrixSubService gamerHintMatrixSubService;
    private final ReadyMadeManager readyMadeManager;

    // 데이터 수집 => API 테스트 성공
    // 개인적으로는 의미상 Post 보다는 Put이 더 가까우므로 PutMapping 하려고 했으나.
    // 아예 없을때는 update 보다는 insert가 되기도 하고.
    // 로그서버 맡은 팀원이 Post로 받아달라고 요청하였음.
    @PostMapping("/internal/hints/data")
    public ResponseEntity<Map<String, Object>> insertData(
            @RequestBody LogServerRequestDto dto
    ) throws Exception {
        try {
            log.debug("insertData() called!!");
            List<MatrixDocument> result = gamerHintMatrixService.insertData(dto);
            return handleSuccess(result);
        } catch (Exception e) {
            return handleError(e.getMessage());
        }
    }

    // 데이터 수집 => API 테스트 성공
    @GetMapping("/hints")
    public ResponseEntity<Map<String, Object>> getIndicator() throws Exception {
        try {
            log.debug("getIndicator() called!!");

            // 아직 보낼게 안 채워졌으면, 기존의 getIndicator로 채우고, 그거 보내기.
            // 안 채워진 예상 사유. 서버 재실행후, 배치 스케줄러 미실행된 공백기.
            if(readyMadeManager.getGetIndicator().isEmpty()){
                List<MatrixDocument> result = gamerHintMatrixService.getIndicator();
                if(result==null){
                    // 클라이언트에서 cold start에 대한 판단을 isOk = false, statusCode = 200으로 하기로 함.
                    Map<String, Object> map = new HashMap<>();
                    map.put("isOk", false);
                    map.put("data", "sorry, now cold start");
                    return new ResponseEntity<Map<String,Object>>(map, HttpStatus.OK);
//                    return handleError("sorry, now cold start");
                } else{
                    // result가 not null일때 업데이트!
                    log.debug("YES DB, YES UPDATE");
                    readyMadeManager.updateGetIndicator(result);
                    return handleSuccess(result);
                }
            }
            // 보낼게 있다! DB 안찍고 이거 바로 보내기.
            else{
                log.debug("NO DB");
                return handleSuccess(readyMadeManager.getGetIndicator());
            }
        } catch (Exception e) {
            return handleError(e.getMessage());
        }
    }

    // 데이터 반출 => API 테스트 성공
    @GetMapping("/hints/pickrate")
    public ResponseEntity<Map<String, Object>> getIndicatorAllPick() throws Exception {
        try {
            log.debug("getIndicatorAllPick: {}");
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
            log.debug("getIndicatorAllWin: {}");
            List<WinMatrixDocument> result = gamerHintMatrixService.getIndicatorAllWin();
            return handleSuccess(result);
        } catch (Exception e) {
            return handleError(e.getMessage());
        }
    }

    // doc 시추 => API 테스트 성공
    @GetMapping("/doc/{patchVersion}/{batchCount}")
    public ResponseEntity<Map<String, Object>> getDoc(
            @PathVariable String patchVersion,
            @PathVariable Integer batchCount
    ) throws Exception {
        try {
            log.debug("getDoc");
            List<MatrixDocument> result = gamerHintMatrixSubService.getDocumentByBatch(patchVersion,batchCount);
            return handleSuccess(result);
        } catch (Exception e) {
            return handleError(e.getMessage());
        }
    }


}
