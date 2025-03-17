// dto/request/create_room_request.h
#pragma once
#include <string>

namespace game_server {

    struct CreateRoomRequest {
        std::string room_name;
        int user_id;
    };

} // namespace game_server