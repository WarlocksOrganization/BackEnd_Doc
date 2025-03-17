// dto/request/login_request.h
#pragma once
#include <string>

namespace game_server {

    struct LoginRequest {
        std::string user_name;
        std::string password;

        LoginRequest(const std::string& username, const std::string& password)
            : user_name(username), password(password) {
        }
    };

} // namespace game_server