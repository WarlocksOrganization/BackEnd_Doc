#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <functional>

class RequestRouter {
public:
    // 라우팅 핸들러 타입 정의 (JSON 요청을 받아 JSON 응답을 반환하는 함수)
    using RouteHandler = std::function<nlohmann::json(const nlohmann::json&)>;

    // 특정 액션에 대한 라우팅 핸들러 등록
    void registerRoute(const std::string& action, RouteHandler handler);

    // 들어온 요청을 적절한 핸들러로 라우팅
    nlohmann::json route(const nlohmann::json& request);

private:
    // 액션과 해당 핸들러 함수를 매핑하는 라우팅 테이블
    std::unordered_map<std::string, RouteHandler> routes_;
};