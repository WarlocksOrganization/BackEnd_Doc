#pragma once
#include <string>

namespace game_server {

    struct RoomUsers {
        int room_id;
        int user_id;
        std::string joined_at;
    };

} // namespace game_server