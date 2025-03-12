// entity/room.h
#pragma once
#include <string>
#include <optional>

namespace game_server {

    struct Room {
        int roomId;
        std::string roomName;
        int creatorId;
        std::string createdAt;
        std::optional<std::string> closedAt;
        int maxPlayers;
        int mapId;
        std::string status;  // 'open', 'closed', 'in_game'
    };

} // namespace game_server