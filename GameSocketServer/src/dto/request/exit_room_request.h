// dto/request/join_room_request.h
#pragma once
#include <string>

namespace game_server {

    struct ExitRoomRequest {
        int userId;
        int roomId;
    };

} // namespace game_server