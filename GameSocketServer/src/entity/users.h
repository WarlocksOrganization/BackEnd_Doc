#pragma once
#include <string>

namespace game_server {

    struct Users {
        int user_id;
        std::string username;
        std::string password_hash;
        int wins;
        int games_played;
        int total_kills;
        int total_damages;
        int total_deaths;
        int rating;
        int highest_rating;
        std::string created_at;
        std::string last_login;
    };

} // namespace game_server