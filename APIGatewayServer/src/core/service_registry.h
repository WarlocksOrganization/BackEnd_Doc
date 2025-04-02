#pragma once
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>
#include "service_endpoint.h"

class ServiceRegistry {
public:
    // 새로운 서비스 엔드포인트를 레지스트리에 등록
    void registerService(const ServiceEndpoint& endpoint);

    // 특정 서비스 타입의 엔드포인트 정보를 조회
    ServiceEndpoint getService(const std::string& service_type);

    // 현재 부하를 고려하여 최적의 서비스 엔드포인트 선택
    ServiceEndpoint getOptimalService(const std::string& service_type);

    // 서비스의 상태를 업데이트 (헬스 체크 결과 반영)
    void updateServiceHealth(const std::string& service_name, bool is_healthy);

private:
    // 서비스 타입별 다중 엔드포인트 매핑
    std::unordered_map<std::string, std::vector<ServiceEndpoint>> services_;

    // 동시 접근 제어를 위한 뮤텍스
    std::mutex registry_mutex_;
};