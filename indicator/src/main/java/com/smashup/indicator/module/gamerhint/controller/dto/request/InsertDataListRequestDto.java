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
public class InsertDataListRequestDto {
    private List<InsertDataRequestDto> list;
}
