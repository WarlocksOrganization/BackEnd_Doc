// dto/request/create_room_request.h
#pragma once
#include <string>

namespace game_server {

    struct CreateRoomRequest {
        std::string roomName;
        int maxPlayers;
        std::string gameMode;
    };

} // namespace game_server