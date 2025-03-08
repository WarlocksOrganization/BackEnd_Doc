// entity/user.h
#pragma once
#include <string>
#include <optional>

namespace game_server {

    struct User {
        int userId;
        std::string username;
        std::string passwordHash;
        int rating;
        int totalGames;
        int totalWins;
        std::string createdAt;
        std::optional<std::string> lastLogin;
    };

} // namespace game_server