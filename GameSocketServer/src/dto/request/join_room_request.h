// dto/request/join_room_request.h
#pragma once
#include <string>

namespace game_server {

    struct JoinRoomRequest {
        int user_Id;
        int room_Id;
    };

} // namespace game_server