// dto/request/register_request.h
#pragma once
#include <string>

namespace game_server {

    struct RegisterRequest {
        std::string user_name;
        std::string password;

        RegisterRequest(const std::string& username, const std::string& password)
            : user_name(username), password(password) {
        }
    };

} // namespace game_server