package com.smashup.indicator.module.version.controller.dto.request;

import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Data;
import lombok.NoArgsConstructor;

@Data
@AllArgsConstructor
@NoArgsConstructor
@Builder
public class UpdatePatchVersionRequestDto {
    private String newPatchVersion;
    private Integer latestBatchCount;
}
