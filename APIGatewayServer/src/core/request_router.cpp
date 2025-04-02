#include "request_router.h"
#include <spdlog/spdlog.h>

void RequestRouter::registerRoute(const std::string& action, RouteHandler handler) {
    routes_[action] = handler;
}

nlohmann::json RequestRouter::route(const nlohmann::json& request) {
    // 요청에서 액션 추출
    if (!request.contains("action")) {
        spdlog::warn("Request has no action key");
        return { {"status", "error"}, {"message", "No action specified"} };
    }

    std::string action = request["action"];

    // 해당 액션에 대한 핸들러 찾기
    auto it = routes_.find(action);
    if (it == routes_.end()) {
        spdlog::warn("No handler for action: {}", action);
        return { {"status", "error"}, {"message", "Unknown action"} };
    }

    // 핸들러 호출 및 응답 반환
    try {
        return it->second(request);
    }
    catch (const std::exception& e) {
        spdlog::error("Error processing action {}: {}", action, e.what());
        return { {"status", "error"}, {"message", "Internal server error"} };
    }
}