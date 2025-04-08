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
import java.util.Comparator;
import java.util.List;
import java.util.Map;

@Service
@RequiredArgsConstructor
public class MasterHintMatrixCardService {
    // 의존성 주입
    private final GamerHintMatrixSubService gamerHintMatrixSubService;
    private final MatrixRepository matrixRepository;
    private final WinMatrixRepository winMatrixRepository;
    private final PoolManager poolManager;
    private final VersionService versionService;


    // 직업간 리밸런스 지표 계산하기
    @Transactional
    public List<RebalanceResponseDto> generateRebalanceCard(int classCode) throws Exception {
        // 제출할 것들 저장할 리스트 생성
        List<RebalanceResponseDto> results = new ArrayList<>();

        // 클래스별로 메타데이터 세팅
        Map<Integer, List<Integer>> classCardPoolMap = poolManager.getClassCardPoolMap();
        List<Integer> classCardPool = classCardPoolMap.get(classCode);

        // 카드별로 기본 세팅.
        for(int cardId : classCardPool){
            // id => index 변환
//            int cardIndex = classCardPoolIndex.get(cardId);
            // cardName도 Set하기.
            String cardName = poolManager.getAllCardPoolMap().get(cardId);
            //
            RebalanceResponseDto dto = RebalanceResponseDto.builder()
                    .type("card")
                    .id(cardId)
                    .name(cardName)
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
        // 최신화 => 이것도 뭔가 getBatchCount-1 하고 싶은데. 1일때 예외처리 하기가 번거롭네.
        List<MatrixDocument> pickMatrix = gamerHintMatrixSubService.getDocumentByBatch(
                versionService.getCurrentPatchVersion(),versionService.getBatchCount());
        List<WinMatrixDocument> winMatrix = gamerHintMatrixSubService.getWinDocumentByBatch(
                versionService.getCurrentPatchVersion(),versionService.getBatchCount());

        // setPick 작업완료.
        setPick(results, pickMatrix, classCode);
        // setWin 작업완료.
        setWin(results, winMatrix, classCode);

        // A직업이 A카드를 선택했을때 우승한 확률도 계산하기.
        for(RebalanceResponseDto result : results){
            Integer pick = result.getPick().getUp();
            Integer win = result.getWin().getUp();
            result.setWinWhenPick( (double) win / pick );
        }
        Comparator<RebalanceResponseDto> comparator1 = Comparator.<RebalanceResponseDto, Double>comparing(o -> o.getWinWhenPick()).reversed();
        Comparator<RebalanceResponseDto> comparator2 = Comparator.<RebalanceResponseDto, Double>comparing(o -> o.getPick().getRate()).reversed();

        results.sort(comparator1.thenComparing(comparator2));
        return results;
    }


    @Transactional
    public void setPick(
            List<RebalanceResponseDto> results,
            List<MatrixDocument> pickMatrix,
            int classCode) throws Exception {
        // cardId => cardIndex로 변환할 재료 미리 로딩.
        Map<Integer, Map<Integer, Integer>> classCardPoolIndexMap = poolManager.getClassCardPoolIndexMap();
        Map<Integer, Integer> classCardPoolIndex = classCardPoolIndexMap.get(classCode);

        // 카드별로 픽수 구해서 분자에 넣고, 모든 카드의 픽수를 더해서 분모에 채우기.
        Integer totalPickCount = 0;
        for(MatrixDocument doc : pickMatrix){
            // T 타입 거르기.
            if(doc.getType().equals("T")){
                continue;
            }
            // 다른 classCode 거르기.
            String classCodeStr = doc.getId().split("/")[3];
            if(classCodeStr.equals( classCode +"" )==false){
                continue;
            }

            // 맞는 classCode와 C 타입 선택됨.
            Integer total = 0;
            // "-1/-1" 사용
            List<List<Integer>> matrix = doc.getMatrixMap().get("-1/-1");
            // cardId 돌면서 찾아서, 픽수 세팅하기.
            for(RebalanceResponseDto result : results){
                int cardId = result.getId();
                int cardIndex = classCardPoolIndex.get(cardId);
                int pickCount = matrix.get(cardIndex).get(cardIndex);
                result.getPick().setUp(pickCount);
                total += pickCount;
            }

            // 그후에. total을 down에 일괄적으로 넣고. Rate 구하기.
            // 해당 직업이 플레이 된 빈도를 분모에 넣어야 하므로 /9 해줌.
            total/=9;
            for(RebalanceResponseDto result : results){
                // 모든 클래스에 total 넣고. 나눈 값 저장하기.
                result.getPick().setDown(total);
                Double rate = (double) result.getPick().getUp() / result.getPick().getDown();
                result.getPick().setRate(rate);

            }

        }
    }

    @Transactional
    public void setWin(
            List<RebalanceResponseDto> results,
            List<WinMatrixDocument> winMatrix,
            int classCode) throws Exception {
        // cardId => cardIndex로 변환할 재료 미리 로딩.
        Map<Integer, Map<Integer, Integer>> classCardPoolIndexMap = poolManager.getClassCardPoolIndexMap();
        Map<Integer, Integer> classCardPoolIndex = classCardPoolIndexMap.get(classCode);

        // 카드별로 픽수 구해서 분자에 넣고, 모든 카드의 픽수를 더해서 분모에 채우기.
        Integer totalPickCount = 0;
        for(WinMatrixDocument doc : winMatrix){
            // T 타입 거르기.
            if(doc.getType().equals("T")){
                continue;
            }
            // 다른 classCode 거르기.
            String classCodeStr = doc.getId().split("/")[3];
            if(classCodeStr.equals( classCode +"" )==false){
                continue;
            }

            // 맞는 classCode와 C 타입 선택됨.
//            Integer total = 0;
            // "-1/-1" 사용
            List<List<Integer>> matrix = doc.getMatrixMap().get("-1/-1");
            // cardId 돌면서 찾아서, 픽수 세팅하기.
            for(RebalanceResponseDto result : results){
                int cardId = result.getId();
                int cardIndex = classCardPoolIndex.get(cardId);
                int winCount = matrix.get(cardIndex).get(cardIndex);
                result.getWin().setUp(winCount);
//                total += winCount;
                //======= pick이 이미 세팅된 상태에서 호출됨. pick의 up을 win의 down에 세팅하기.
                int pickCount = result.getPick().getUp();
                result.getWin().setDown(pickCount);
                Double rate = (double) winCount / pickCount;
                result.getWin().setRate(rate);
            }

        }
    }


}
