package com.smashup.indicator.common.util;

import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;

import java.util.HashMap;
import java.util.Map;

public class AbstractRestController {
    protected ResponseEntity<Map<String, Object>> handleSuccess(Object data){
        Map<String, Object> map = new HashMap<>();
        map.put("isOk", true);
        map.put("data", data);
        return new ResponseEntity<Map<String,Object>>(map, HttpStatus.OK);
    }
    protected ResponseEntity<Map<String, Object>> handleError(Object data){
        Map<String, Object> map = new HashMap<>();
        map.put("isOk", false);
        map.put("data", data);
        return new ResponseEntity<Map<String,Object>>(map, HttpStatus.INTERNAL_SERVER_ERROR);
    }
}
