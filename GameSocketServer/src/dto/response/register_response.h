// dto/response/register_response.h
#pragma once
#include <string>

namespace game_server {

    struct RegisterResponse {
        bool success;
        std::string message;
        int userId = 0;
        std::string username;
    };

} // namespace game_server