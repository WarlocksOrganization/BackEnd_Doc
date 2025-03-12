// dto/request/join_room_request.h
#pragma once
#include <string>

namespace game_server {

    struct JoinRoomRequest {
        int userId;
        int roomId;
    };

} // namespace game_server