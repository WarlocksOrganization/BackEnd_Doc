// dto/response/login_response.h
#pragma once
#include <string>

namespace game_server {

    struct LoginResponse {
        bool success;
        std::string message;
        int userId = 0;
        std::string username;
        int rating = 0;
        int totalGames = 0;
        int totalWins = 0;
    };

} // namespace game_server