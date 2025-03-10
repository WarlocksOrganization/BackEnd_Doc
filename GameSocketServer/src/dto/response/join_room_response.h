// dto/response/join_room_response.h
#pragma once
#include <string>
#include <vector>

namespace game_server {

    struct JoinRoomResponse {
        bool success;
        std::string message;
        int roomId;
        std::string roomName;
        int currentPlayers;
        int maxPlayers;
        std::string gameMode;
        std::vector<int> playerIds;
    };

} // namespace game_server