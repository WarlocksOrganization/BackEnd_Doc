package com.smashup.indicator.module.version;

import com.smashup.indicator.module.gamerhint.domain.entity.MatrixDocument;
import com.smashup.indicator.module.gamerhint.domain.entity.WinMatrixDocument;
import com.smashup.indicator.module.gamerhint.repository.MatrixRepository;
import com.smashup.indicator.module.gamerhint.repository.WinMatrixRepository;
import com.smashup.indicator.module.gamerhint.service.impl.GamerHintMatrixService;
import com.smashup.indicator.module.gamerhint.service.impl.GamerHintMatrixSubService;
import com.smashup.indicator.module.version.service.impl.VersionService;
import lombok.RequiredArgsConstructor;
import org.springframework.scheduling.annotation.Scheduled;
import org.springframework.stereotype.Component;
import org.springframework.stereotype.Service;

import java.util.List;

@RequiredArgsConstructor
@Service

public class BatchCountScheduler {
    private final GamerHintMatrixSubService gamerHintMatrixSubService;
    private final VersionService versionService;
    private final MatrixRepository matrixRepository;
    private final WinMatrixRepository winMatrixRepository;



//    @Scheduled(fixedRate = 1000*60*60*1) // 1시간마다 실행
    @Scheduled(fixedRate = 1000*60) // 1시간마다 실행
    public synchronized void incrementBatchCount() {
//        System.out.println("⏰ 스케줄러 진입");
        try {
            // 배치카운트 ++
            versionService.incrementBatchCount();
            // 직전 배치카운트 document 가져와서 null 체크 후 early return
            List<MatrixDocument> docs = gamerHintMatrixSubService.getDocumentByBatch(versionService.getCurrentPatchVersion(), versionService.getBatchCount()-1);
            List<WinMatrixDocument> winDocs = gamerHintMatrixSubService.getWinDocumentByBatch(versionService.getCurrentPatchVersion(), versionService.getBatchCount()-1);

            if (docs == null || winDocs == null) {
                System.err.println("[WARN] setDocument returned null.");
                return;
            }
            // 직전 배치카운트 가져와서 현재 배치카운트에 맞게 각종 필드값 바꾸기.
            for (MatrixDocument doc: docs) {
                // 현재(새로 바뀐) 배치카운트 사용해서 id 교체.
                String id = String.join("/", versionService.getCurrentPatchVersion(),versionService.getBatchCount()+"",doc.getType() );
                doc.setId(id);
                // version 처음부터 시작하니까 교체. 1L => 0L => null
                doc.setVersion(null);
                // 저장.
                matrixRepository.save(doc);
            }
            // win버전
            for (WinMatrixDocument winDoc: winDocs) {
                // 현재(새로 바뀐) 배치카운트 사용해서 id 교체.
                String id = String.join("/", versionService.getCurrentPatchVersion(),versionService.getBatchCount()+"",winDoc.getType() );
                winDoc.setId(id);
                // version 처음부터 시작하니까 교체. 1L => 0L => null
                winDoc.setVersion(null);
                // 저장.
                winMatrixRepository.save(winDoc);
            }


        } catch (Exception e) {
            System.err.println("[ERROR] Failed to process scheduled incrementBatchCount:");
            e.printStackTrace();
        }

    }
}
