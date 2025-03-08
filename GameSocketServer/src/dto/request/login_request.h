// dto/request/login_request.h
#pragma once
#include <string>

namespace game_server {

    struct LoginRequest {
        std::string username;
        std::string password;

        LoginRequest(const std::string& username, const std::string& password)
            : username(username), password(password) {
        }
    };

} // namespace game_server