package com.smashup.indicator.module.version.controller.dto.request;

import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Data;
import lombok.NoArgsConstructor;

import java.util.List;

@Data
@AllArgsConstructor
@NoArgsConstructor
@Builder
public class UpdateCardPoolRequestDto {
    private List<Integer> cardPool;
    private String timeStamp;
}
