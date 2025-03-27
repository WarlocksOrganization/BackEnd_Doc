package com.smashup.indicator.module.version;

import org.springframework.scheduling.annotation.Scheduled;
import org.springframework.stereotype.Component;

@Component
public class BatchCountManager {
    private int batchCount = 1;

    public synchronized void resetBatchCount() {
        this.batchCount = 1; // PATCH_VERSION이 바뀌면 초기화
    }

    public synchronized void plusBatchCount() {
        this.batchCount++; // PATCH_VERSION이 바뀌면 초기화
    }

    @Scheduled(fixedRate = 60*60*1000) // 1시간마다 실행
    public synchronized void incrementBatchCount() {
        this.batchCount++;
    }

    public int getBatchCount() {
        return batchCount;
    }
}
