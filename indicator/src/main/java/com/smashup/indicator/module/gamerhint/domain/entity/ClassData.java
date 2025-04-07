package com.smashup.indicator.module.gamerhint.domain.entity;

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
public class ClassData {
    private int classCode;
    private int playCount;
    private List<Integer> cardPool;
//    private List<Map<String, List<Integer>>> couplingCount; // 객체 배열.// 역직렬화 이후 정렬하기 편하게 하려고.
//    private List<List<Integer>> couplingCount; // 2차원 배열.
}
