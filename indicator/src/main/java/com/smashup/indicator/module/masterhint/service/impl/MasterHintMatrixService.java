package com.smashup.indicator.module.masterhint.service.impl;

import com.smashup.indicator.module.gamerhint.domain.entity.MatrixDocument;
import com.smashup.indicator.module.gamerhint.domain.entity.WinMatrixDocument;
import com.smashup.indicator.module.gamerhint.repository.MatrixRepository;
import com.smashup.indicator.module.gamerhint.repository.WinMatrixRepository;
import com.smashup.indicator.module.gamerhint.service.impl.GamerHintMatrixSubService;
import com.smashup.indicator.module.masterhint.controller.dto.response.CountResponseDto;
import com.smashup.indicator.module.masterhint.controller.dto.response.RebalanceResponseDto;
import com.smashup.indicator.module.version.PoolManager;
import com.smashup.indicator.module.version.service.impl.VersionService;
import lombok.RequiredArgsConstructor;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.util.ArrayList;
import java.util.List;

@Service
@RequiredArgsConstructor
public class MasterHintMatrixService {
    // 의존성 주입
    private final GamerHintMatrixSubService gamerHintMatrixSubService;
    private final MatrixRepository matrixRepository;
    private final WinMatrixRepository winMatrixRepository;
    private final PoolManager poolManager;
    private final VersionService versionService;


    // 직업간 리밸런스 지표 계산하기
    @Transactional
    public List<RebalanceResponseDto> generateRebalanceClass() throws Exception {
        // 제출할 것들 저장할 리스트 생성
        List<RebalanceResponseDto> results = new ArrayList<>();

        for(int classCode : poolManager.getClassPool()){
            RebalanceResponseDto dto = RebalanceResponseDto.builder()
                    .type("class")
                    .id(classCode)
                    .pick(CountResponseDto.builder()
                            .down(0)
                            .up(0)
                            .rate(0D)
                            .build())
                    .win(CountResponseDto.builder()
                            .down(0)
                            .up(0)
                            .rate(0D)
                            .build())
                    .build();
            results.add(dto);
        }
        List<MatrixDocument> pickMatrix = gamerHintMatrixSubService.getDocumentByBatch(versionService.getCurrentPatchVersion(),versionService.getBatchCount());
        List<WinMatrixDocument> winMatrix = gamerHintMatrixSubService.getWinDocumentByBatch(versionService.getCurrentPatchVersion(),versionService.getBatchCount());

        setPick(results, pickMatrix);
        setWin(results, winMatrix);

        // 우승 점유율 말고, A직업을 선택했을때 우승할 확률도 계산하기.
        for(RebalanceResponseDto result : results){
            Integer pick = result.getPick().getUp();
            Integer win = result.getWin().getUp();
            result.setWinWhenPick( (double) win / pick );
        }


        return results;
    }


    @Transactional
    public void setPick(List<RebalanceResponseDto> results, List<MatrixDocument> pickMatrix) throws Exception {
        // 직업별로 픽수 구해서 분자에 넣고, 모든 직업의 픽수를 더해서 분모에 채우기.
        Integer totalPickCount = 0;
        for(MatrixDocument doc : pickMatrix){
            // T 타입 거르기.
            if(doc.getType().equals("T")){
                continue;
            }
            // 클래스 코드 추출
            String classCodeStr = doc.getId().split("/")[3];
            Integer classCode = Integer.parseInt(classCodeStr);
            Integer pickCount = calculatePick(doc);
            // pickCount 총합 구하기.
            totalPickCount += pickCount;
            // 해당 클래스의 픽수에 넣어주기.
            for(RebalanceResponseDto result : results){
                // 다른 클래스는 패스
                if(result.getId()!=classCode){
                    continue;
                }
                result.getPick().setUp(pickCount);
            }

        }
        // 그후에. total을 down에 일괄적으로 넣고. Rate 구하기.
        for(RebalanceResponseDto result : results){
            // 모든 클래스에 total 넣고. 나눈 값 저장하기.
            result.getPick().setDown(totalPickCount);
            Double rate = (double) result.getPick().getUp() / result.getPick().getDown();
            result.getPick().setRate(rate);
        }
    }
    @Transactional
    public void setWin(List<RebalanceResponseDto> results, List<WinMatrixDocument> winMatrix) throws Exception {
        // 직업별로 픽수 구해서 분자에 넣고, 모든 직업의 픽수를 더해서 분모에 채우기.
        Integer totalWinCount = 0;
        for(WinMatrixDocument winDoc : winMatrix){
            // T 타입 거르기.
            if(winDoc.getType().equals("T")){
                continue;
            }
            // 클래스 코드 추출
            String classCodeStr = winDoc.getId().split("/")[3];
            Integer classCode = Integer.parseInt(classCodeStr);
            Integer winCount = calculateWin(winDoc);
            // winCount 총합 구하기. => 분모는 총합이 아님. => 근데 분모를 우승 총합으로 해도 되고, 해당 직업픽수로 해도 되나?
            totalWinCount += winCount;
            // 해당 클래스의 win수에 넣어주기.
            for(RebalanceResponseDto result : results){
                // 다른 클래스는 패스
                if(result.getId()!=classCode){
                    continue;
                }
                result.getWin().setUp(winCount);
            }

        }
        // 그후에. total을 down에 일괄적으로 넣고. Rate 구하기.
        for(RebalanceResponseDto result : results){
            // 모든 클래스에 total 넣고. 나눈 값 저장하기.
            result.getWin().setDown(totalWinCount);
            Double rate = (double) result.getWin().getUp() / result.getWin().getDown();
            result.getWin().setRate(rate);
        }
    }



    @Transactional
    public Integer calculatePick(MatrixDocument doc) throws Exception {
        Integer result = 0;
        // "-1/-1" 사용
//        Map<String, List<List<Integer>>> matrix = doc.getMatrixMap();
        List<List<Integer>> matrix = doc.getMatrixMap().get("-1/-1");
        // rowLen = colLen+1 이므로. colLen을 사용.
        int colLen = matrix.get(0).size();
        for (int i = 0; i < colLen; i++) {
            int pickCount = matrix.get(i).get(i);
            result+=pickCount;
        }
        // 다 더했으니까 9로 나누기
        result /= 9;
        return result;
    }
    @Transactional
    public Integer calculateWin(WinMatrixDocument winDoc) throws Exception {
        Integer result = 0;
        // "-1/-1" 사용
        List<List<Integer>> matrix = winDoc.getMatrixMap().get("-1/-1");
        // rowLen = colLen+1 이므로. colLen을 사용.
        int colLen = matrix.get(0).size();
        for (int i = 0; i < colLen; i++) {
            int winCount = matrix.get(i).get(i);
            result+=winCount;
        }
        // 다 더했으니까 9로 나누기
        result /= 9;
        return result;
    }


}
