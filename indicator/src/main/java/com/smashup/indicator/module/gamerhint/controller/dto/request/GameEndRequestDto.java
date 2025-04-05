package com.smashup.indicator.module.gamerhint.controller.dto.request;

import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Data;
import lombok.NoArgsConstructor;

import java.util.List;

@Data
@AllArgsConstructor
@NoArgsConstructor
@Builder
public class GameEndRequestDto {
    // 게임당 공통데이터.
    private String gameId;
    private String patchVersion;
    private Integer mapId;
    private Integer playerCount;
    // 플레이어별 데이터
    private List<PlayerLogRequestDto> playerLogs;
}
