package com.smashup.indicator.module.gamerhint.domain.entity;

import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Data;
import lombok.NoArgsConstructor;
import org.springframework.data.annotation.Id;
import org.springframework.data.mongodb.core.mapping.Document;

import java.util.List;

@Data
@AllArgsConstructor
@NoArgsConstructor
@Builder

@Document(collection = "gamerHints2")
public class GamerHintDocument3 {
    @Id
    private String id; // String으로 동적 생성 // patch+batch

    private List<ClassData> data;
}
