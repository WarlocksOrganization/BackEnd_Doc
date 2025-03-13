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
public class InsertDataRequestDto {
    private Integer classCode;
    
    private List<Integer> round1Set;
    private List<Integer> round2Set;
    private List<Integer> round3Set;

    private Integer rank;
    private Integer score;
}
