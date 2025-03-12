// dto/response/list_rooms_response.h
#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace game_server {

    struct RoomInfo {
        int roomId;
        std::string roomName;
        int creatorId;
        int currentPlayers;
        int maxPlayers;
        std::string status;
        std::string createdAt;
    };

    struct ListRoomsResponse {
        bool success;
        std::string message;
        std::vector<RoomInfo> rooms;

        nlohmann::json toJson() const {
            nlohmann::json j = {
                {"status", success ? "success" : "error"},
                {"message", message},
                {"rooms", nlohmann::json::array()}
            };

            for (const auto& room : rooms) {
                j["rooms"].push_back({
                    {"room_id", room.roomId},
                    {"room_name", room.roomName},
                    {"creator_id", room.creatorId},
                    {"current_players", room.currentPlayers},
                    {"max_players", room.maxPlayers},
                    {"status", room.status},
                    {"created_at", room.createdAt}
                    });
            }

            return j;
        }
    };

} // namespace game_server