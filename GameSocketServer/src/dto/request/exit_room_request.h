// dto/request/join_room_request.h
#pragma once
#include <string>

namespace game_server {

    struct ExitRoomRequest {
        int user_Id;
        int room_id;
    };

} // namespace game_server