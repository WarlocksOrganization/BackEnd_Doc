#pragma once
#include <string>
#include <optional>

namespace game_server {

    struct Rooms {
        int room_id;
        std::string room_name;
        int host_id;
        std::string status;
        std::string ip_address;
        int port;
        int max_players;
        std::string status;
        std::string created_at;
    };

} // namespace game_server