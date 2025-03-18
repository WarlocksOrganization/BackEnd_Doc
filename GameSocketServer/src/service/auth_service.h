// service/auth_service.h
#pragma once
#include <memory>
#include <nlohmann/json.hpp>

namespace game_server {

    class UserRepository;

    class AuthService {
    public:
        virtual ~AuthService() = default;

        virtual nlohmann::json registerUser(const json& request) = 0;
        virtual nlohmann::json loginUser(const json& request) = 0;

        static std::unique_ptr<AuthService> create(std::shared_ptr<UserRepository> userRepo);
    };

} // namespace game_server