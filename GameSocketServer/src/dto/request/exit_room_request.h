// dto/request/join_room_request.h
#pragma once
#include <string>

namespace game_server {

    struct ExitRoomRequest {
        int userId;
    };

} // namespace game_server