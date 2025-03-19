#include "request_router.h"
#include <spdlog/spdlog.h>

void RequestRouter::registerRoute(const std::string& action, RouteHandler handler) {
    routes_[action] = handler;
}

nlohmann::json RequestRouter::route(const nlohmann::json& request) {
    // ��û���� �׼� ����
    if (!request.contains("action")) {
        spdlog::warn("Request has no action key");
        return { {"status", "error"}, {"message", "No action specified"} };
    }

    std::string action = request["action"];

    // �ش� �׼ǿ� ���� �ڵ鷯 ã��
    auto it = routes_.find(action);
    if (it == routes_.end()) {
        spdlog::warn("No handler for action: {}", action);
        return { {"status", "error"}, {"message", "Unknown action"} };
    }

    // �ڵ鷯 ȣ�� �� ���� ��ȯ
    try {
        return it->second(request);
    }
    catch (const std::exception& e) {
        spdlog::error("Error processing action {}: {}", action, e.what());
        return { {"status", "error"}, {"message", "Internal server error"} };
    }
}