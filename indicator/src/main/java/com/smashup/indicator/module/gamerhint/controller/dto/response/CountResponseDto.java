package com.smashup.indicator.module.gamerhint.controller.dto.response;

import com.smashup.indicator.module.gamerhint.controller.dto.request.PlayerLogRequestDto;
import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Data;
import lombok.NoArgsConstructor;

import java.util.List;

@Data
@AllArgsConstructor
@NoArgsConstructor
@Builder
public class CountResponseDto {
    private Integer up;
    private Integer down;
    private Double rate;
}
