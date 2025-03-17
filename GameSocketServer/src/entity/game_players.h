#pragma once
#include<string>

namespace game_server {

    struct Games {
        int game_id;
        std::string room_id;
        int map_id;
        std::string status;
        std::string started_at;
        std::string completed_at;
    };

} // namespace game_server