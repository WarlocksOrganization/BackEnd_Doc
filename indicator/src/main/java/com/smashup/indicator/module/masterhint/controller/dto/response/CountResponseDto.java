package com.smashup.indicator.module.masterhint.controller.dto.response;

import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Data;
import lombok.NoArgsConstructor;

@Data
@AllArgsConstructor
@NoArgsConstructor
@Builder
public class CountResponseDto {
    private Integer up;
    private Integer down;
    private Double rate;
}
