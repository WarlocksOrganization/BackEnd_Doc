// dto/response/join_room_response.h
#pragma once
#include <string>

namespace game_server {

    struct ExitRoomResponse {
        bool success;
        std::string message;
        int roomId;
    };

} // namespace game_server