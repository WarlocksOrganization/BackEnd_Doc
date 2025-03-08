// controller/auth_controller.cpp
#include "auth_controller.h"
#include "../dto/request/login_request.h"
#include "../dto/request/register_request.h"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

namespace game_server {

    using json = nlohmann::json;

    AuthController::AuthController(std::shared_ptr<AuthService> authService)
        : authService_(authService) {
    }

    std::string AuthController::handleRequest(const json& request) {
        std::string action = request["action"];

        if (action == "register") {
            return handleRegister(request);
        }
        else if (action == "login") {
            return handleLogin(request);
        }
        else {
            json error_response = {
                {"status", "error"},
                {"message", "Unknown auth action"}
            };
            return error_response.dump();
        }
    }

    std::string AuthController::handleRegister(const json& request) {
        try {
            // 요청 데이터 추출
            RegisterRequest registerRequest{
                request["username"].get<std::string>(),
                request["password"].get<std::string>()
            };

            // 서비스 호출
            auto response = authService_->registerUser(registerRequest);

            // 응답 생성
            json jsonResponse = {
                {"status", response.success ? "success" : "error"},
                {"message", response.message}
            };

            if (response.success) {
                jsonResponse["user_id"] = response.userId;
                jsonResponse["username"] = response.username;
            }

            return jsonResponse.dump();
        }
        catch (const std::exception& e) {
            spdlog::error("Error during registration: {}", e.what());
            json error_response = {
                {"status", "error"},
                {"message", "Internal server error"}
            };
            return error_response.dump();
        }
    }

    std::string AuthController::handleLogin(const json& request) {
        try {
            // 요청 데이터 추출
            LoginRequest loginRequest{
                request["username"].get<std::string>(),
                request["password"].get<std::string>()
            };

            // 서비스 호출 
            auto response = authService_->loginUser(loginRequest);

            // 응답 생성
            json jsonResponse = {
                {"status", response.success ? "success" : "error"},
                {"message", response.message}
            };

            if (response.success) {
                jsonResponse["user_id"] = response.userId;
                jsonResponse["username"] = response.username;
                jsonResponse["rating"] = response.rating;
                jsonResponse["total_games"] = response.totalGames;
                jsonResponse["total_wins"] = response.totalWins;
            }

            return jsonResponse.dump();
        }
        catch (const std::exception& e) {
            spdlog::error("Error during login: {}", e.what());
            json error_response = {
                {"status", "error"},
                {"message", "Internal server error"}
            };
            return error_response.dump();
        }
    }

} // namespace game_server