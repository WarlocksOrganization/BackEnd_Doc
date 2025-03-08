// controller/auth_controller.h
#pragma once
#include "controller.h"
#include "../service/auth_service.h"
#include <memory>

namespace game_server {

    class AuthController : public Controller {
    public:
        explicit AuthController(std::shared_ptr<AuthService> authService);
        ~AuthController() override = default;

        std::string handleRequest(const nlohmann::json& request) override;

    private:
        std::string handleRegister(const nlohmann::json& request);
        std::string handleLogin(const nlohmann::json& request);

        std::shared_ptr<AuthService> authService_;
    };

} // namespace game_server