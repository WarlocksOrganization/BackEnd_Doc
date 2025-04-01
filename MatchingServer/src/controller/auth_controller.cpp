// controller/auth_controller.cpp
// ?몄쬆 而⑦듃濡ㅻ윭 援ы쁽 ?뚯씪
// ?ъ슜???깅줉 諛?濡쒓렇???붿껌??泥섎━?섎뒗 而⑦듃濡ㅻ윭
#include "auth_controller.h"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

namespace game_server {

    using json = nlohmann::json;

    AuthController::AuthController(std::shared_ptr<AuthService> authService)
        : authService_(authService) {
    }

    nlohmann::json AuthController::handleRequest(json& request) {
        // ?붿껌??action ?꾨뱶???곕씪 ?곸젅???몃뱾???몄텧
        std::string action = request["action"];

        if (action == "register") {
            return handleRegister(request);
        }
        else if (action == "login") {
            return handleLogin(request);
        }
        else if (action == "SSAFYlogin") {
            return handleRegisterCheckAndLogin(request);
        }
        else if (action == "updateNickName") {
            return handleUpdateNickName(request);
        }
        else {
            json error_response = {
                {"status", "error"},
                {"message", "Unknown auth action"}
            };
            return error_response;
        }
    }

    nlohmann::json AuthController::handleRegister(json& request) {
        // ?쒕퉬??怨꾩링 ?몄텧?섏뿬 ?ъ슜???깅줉 ?섑뻾
        json response = authService_->registerUser(request);
        return response;
    }

    nlohmann::json AuthController::handleLogin(json& request) {
        json response = authService_->loginUser(request);
        return response;
    }

    nlohmann::json AuthController::handleRegisterCheckAndLogin(nlohmann::json& request) {
        json response = authService_->registerCheckAndLogin(request);
        return response;
    }

    nlohmann::json AuthController::handleUpdateNickName(nlohmann::json& request) {
        json response = authService_->updateNickName(request);
        return response;
    }

} // namespace game_server
