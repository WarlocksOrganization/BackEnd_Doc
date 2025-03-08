// dto/request/register_request.h
#pragma once
#include <string>

namespace game_server {

    struct RegisterRequest {
        std::string username;
        std::string password;

        RegisterRequest(const std::string& username, const std::string& password)
            : username(username), password(password) {
        }
    };

} // namespace game_server