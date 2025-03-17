// dto/response/join_room_response.h
#pragma once
#include <string>

namespace game_server {

    struct JoinRoomResponse {
        bool success;
        std::string message;
        int room_id;
        std::string room_name;
        int host_id;
        std::string ip_address;
        int port;
        int max_players;
        std::string created_at;
    };

} // namespace game_server