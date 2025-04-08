package com.smashup.indicator.module.version;

import com.smashup.indicator.module.gamerhint.domain.entity.MatrixDocument;
import com.smashup.indicator.module.gamerhint.service.impl.GamerHintMatrixService;
import lombok.RequiredArgsConstructor;
import org.springframework.stereotype.Component;

import java.util.*;

@Component
@RequiredArgsConstructor
public class ReadyMadeManager {
    // 의존성 주입
    private final GamerHintMatrixService gamerHintMatrixService;


    // 필드 변수
    private final List<MatrixDocument> getIndicator = new ArrayList<>();

    // 배치 스케줄러가 호출할때마다 업데이트.
    public synchronized void updateGetIndicator(List<MatrixDocument> data) {
        getIndicator.clear();
//        List<MatrixDocument> temp=null;
//        try {
//            temp = gamerHintMatrixService.getIndicator();
//        } catch (Exception e) {
//            throw new RuntimeException(e);
//        }
        getIndicator.addAll(data);
    }

    // Controller에서 가져올때 사용.
    public List<MatrixDocument> getGetIndicator() {
        return Collections.unmodifiableList(getIndicator);
    }

}
