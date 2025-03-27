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
public class UpdatePoolRequestDto {
    private List<Integer> cardPool;
    private List<Integer> classPool;
    private List<Integer> mapPool;
    private List<Integer> playerNumPool;
    private String timeStamp;
}
