// entity/room_player.h
#pragma once
#include <string>
#include <optional>

namespace game_server {

    struct RoomPlayer {
        int id;
        int roomId;
        int userId;
        std::string joinTime;
        std::optional<std::string> leaveTime;
    };

} // namespace game_server