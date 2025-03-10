// dto/response/create_room_response.h
#pragma once
#include <string>

namespace game_server {

    struct CreateRoomResponse {
        bool success;
        std::string message;
        int roomId;
        std::string roomName;
    };

} // namespace game_server