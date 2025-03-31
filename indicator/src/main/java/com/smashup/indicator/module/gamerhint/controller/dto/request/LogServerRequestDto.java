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
public class LogServerRequestDto {
    // 여러 게임의 data 모아서 받기
    private List<GameEndRequestDto> data;
}
