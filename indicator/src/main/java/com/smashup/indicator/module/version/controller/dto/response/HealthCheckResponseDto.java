package com.smashup.indicator.module.version.controller.dto.response;

import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Data;
import lombok.NoArgsConstructor;

import java.util.List;
import java.util.Map;

@Data
@AllArgsConstructor
@NoArgsConstructor
@Builder
public class HealthCheckResponseDto {
    private String serverPatchVersion;
    private Integer serverBatchCount;

}
