// service/auth_service.h
#pragma once
#include "../dto/request/register_request.h"
#include "../dto/response/register_response.h"
#include "../dto/request/login_request.h"
#include "../dto/response/login_response.h"
#include <memory>

namespace game_server {

    class UserRepository;

    class AuthService {
    public:
        virtual ~AuthService() = default;

        virtual RegisterResponse registerUser(const RegisterRequest& request) = 0;
        virtual LoginResponse loginUser(const LoginRequest& request) = 0;

        // 팩토리 메서드
        static std::unique_ptr<AuthService> create(std::shared_ptr<UserRepository> userRepo);
    };

} // namespace game_server