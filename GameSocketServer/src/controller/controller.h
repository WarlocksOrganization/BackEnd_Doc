// controller/controller.h
#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace game_server {

    class Controller {
    public:
        virtual ~Controller() = default;

        // 요청 처리 메서드
        virtual std::string handleRequest(const nlohmann::json& request) = 0;
    };

} // namespace game_server