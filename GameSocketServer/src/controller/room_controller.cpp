// controller/room_controller.cpp
#include "room_controller.h"
#include "../dto/request/create_room_request.h"
#include "../dto/request/join_room_request.h"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

namespace game_server {

    using json = nlohmann::json;

    RoomController::RoomController(std::shared_ptr<RoomService> roomService)
        : roomService_(roomService) {
    }

    std::string RoomController::handleRequest(const json& request) {
        std::string action = request["action"];

        if (action == "create_room") {
            return handleCreateRoom(request);
        }
        else if (action == "join_room") {
            return handleJoinRoom(request);
        }
        else if (action == "list_rooms") {
            return handleListRooms(request);
        }
        else {
            json error_response = {
                {"status", "error"},
                {"message", "Unknown room action"}
            };
            return error_response.dump();
        }
    }

    std::string RoomController::handleCreateRoom(const json& request) {
        try {
            int userId = 0;
            if (request.contains("user_id")) {
                userId = request["user_id"];
            }
            if (userId <= 0) {
                json error_response = {
                    {"status", "error"},
                    {"message", "Invalid user ID for room creation"}
                };
                return error_response.dump();
            }

            // Create room request object
            CreateRoomRequest createRoomRequest{
                request["room_name"].get<std::string>(),
                request["max_players"].get<int>(),
                request["game_mode"].get<std::string>(),
            };

            // Call service
            auto response = roomService_->createRoom(createRoomRequest, userId);

            // Create response
            json jsonResponse = {
                {"status", response.success ? "success" : "error"},
                {"message", response.message}
            };

            if (response.success) {
                jsonResponse["room_id"] = response.roomId;
                jsonResponse["room_name"] = response.roomName;
            }

            return jsonResponse.dump();
        }
        catch (const std::exception& e) {
            spdlog::error("Error creating room: {}", e.what());
            json error_response = {
                {"status", "error"},
                {"message", "Internal server error"}
            };
            return error_response.dump();
        }
    }

    std::string RoomController::handleJoinRoom(const json& request) {
        try {
            int userId = 0;
            if (request.contains("user_id")) {
                userId = request["user_id"];
            }
            if (userId <= 0) {
                json error_response = {
                    {"status", "error"},
                    {"message", "Invalid user ID for joining room"}
                };
                return error_response.dump();
            }

            // Create join room request object
            JoinRoomRequest joinRoomRequest{
                request["room_id"].get<int>()
            };

            // Call service
            auto response = roomService_->joinRoom(joinRoomRequest, userId);

            // Create response
            json jsonResponse = {
                {"status", response.success ? "success" : "error"},
                {"message", response.message}
            };

            if (response.success) {
                jsonResponse["room_id"] = response.roomId;
                jsonResponse["room_name"] = response.roomName;
                jsonResponse["current_players"] = response.currentPlayers;
                jsonResponse["max_players"] = response.maxPlayers;
                jsonResponse["game_mode"] = response.gameMode;
                jsonResponse["player_ids"] = response.playerIds;
            }

            return jsonResponse.dump();
        }
        catch (const std::exception& e) {
            spdlog::error("Error joining room: {}", e.what());
            json error_response = {
                {"status", "error"},
                {"message", "Internal server error"}
            };
            return error_response.dump();
        }
    }

    std::string RoomController::handleListRooms(const json& request) {
        try {
            // Call service
            auto response = roomService_->listRooms();

            // Convert to JSON response
            return response.toJson().dump();
        }
        catch (const std::exception& e) {
            spdlog::error("Error listing rooms: {}", e.what());
            json error_response = {
                {"status", "error"},
                {"message", "Internal server error"}
            };
            return error_response.dump();
        }
    }

} // namespace game_server