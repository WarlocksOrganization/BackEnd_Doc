// controller/auth_controller.cpp
// 인증 컨트롤러 구현 파일
// 사용자 등록 및 로그인 요청을 처리하는 컨트롤러
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
        // 요청의 action 필드에 따라 적절한 핸들러 호출
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
            // 요청 데이터 추출 및 DTO 생성
            RegisterRequest registerRequest{
                request["user_name"].get<std::string>(),
                request["password"].get<std::string>()
            };

            // 서비스 계층 호출하여 사용자 등록 수행
            auto response = authService_->registerUser(registerRequest);

            // 응답 생성
            json jsonResponse = {
                {"status", response.success ? "success" : "error"},
                {"message", response.message}
            };

            // 성공 시 추가 정보 포함
            if (response.success) {
                jsonResponse["user_id"] = response.userId;
                jsonResponse["user_name"] = response.username;
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
            // 요청 데이터 추출 및 DTO 생성
            LoginRequest loginRequest{
                request["user_name"].get<std::string>(),
                request["password"].get<std::string>()
            };

            // 서비스 계층 호출하여 로그인 수행
            auto response = authService_->loginUser(loginRequest);

            // 응답 생성
            json jsonResponse = {
                {"status", response.success ? "success" : "error"},
                {"message", response.message}
            };

            // 성공 시 사용자 정보 포함
            if (response.success) {
                jsonResponse["user_id"] = response.userId;
                jsonResponse["user_name"] = response.username;
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