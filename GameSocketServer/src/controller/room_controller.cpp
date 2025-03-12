// controller/room_controller.cpp
// 방 컨트롤러 구현 파일
// 방 생성, 참가, 목록 조회 등의 요청을 처리하는 컨트롤러
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
        // 요청의 action 필드에 따라 적절한 핸들러 호출
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
            // 사용자 ID 확인
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

            // 방 생성 요청 객체 생성
            CreateRoomRequest createRoomRequest{
                request["room_name"].get<std::string>(),
                request["max_players"].get<int>(),
            };

            // 서비스 계층 호출
            auto response = roomService_->createRoom(createRoomRequest, userId);

            // 응답 생성
            json jsonResponse = {
                {"status", response.success ? "success" : "error"},
                {"message", response.message}
            };

            // 성공 시 방 정보 포함
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
            // 사용자 ID 확인
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

            // 방 참가 요청 객체 생성
            JoinRoomRequest joinRoomRequest{
                request["room_id"].get<int>()
            };

            // 서비스 계층 호출
            auto response = roomService_->joinRoom(joinRoomRequest, userId);

            // 응답 생성
            json jsonResponse = {
                {"status", response.success ? "success" : "error"},
                {"message", response.message}
            };

            // 성공 시 방 정보 및 참가자 목록 포함
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
            // 서비스 계층 호출하여 방 목록 조회
            auto response = roomService_->listRooms();

            // JSON 응답으로 변환
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