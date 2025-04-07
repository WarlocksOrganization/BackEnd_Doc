package com.smashup.indicator.module.masterhint.controller.dto.response;

import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Data;
import lombok.NoArgsConstructor;

@Data
@AllArgsConstructor
@NoArgsConstructor
@Builder
public class RebalanceResponseDto {
    private String type;
    private Integer id;
    private CountResponseDto pick;
    private CountResponseDto win;
    private Double winWhenPick;
}
