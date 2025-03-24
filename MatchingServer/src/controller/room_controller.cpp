// controller/room_controller.cpp
// �� ��Ʈ�ѷ� ���� ����
// �� ����, ����, ��� ��ȸ ���� ��û�� ó���ϴ� ��Ʈ�ѷ�
#include "room_controller.h"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

namespace game_server {

    using json = nlohmann::json;

    RoomController::RoomController(std::shared_ptr<RoomService> roomService)
        : roomService_(roomService) {
    }

    nlohmann::json RoomController::handleRequest(json& request) {
        // ��û�� action �ʵ忡 ���� ������ �ڵ鷯 ȣ��
        std::string action = request["action"];

        if (action == "createRoom") {
            return handleCreateRoom(request);
        }
        else if (action == "joinRoom") {
            return handleJoinRoom(request);
        }
        else if (action == "exitRoom") {
            return handleExitRoom(request);
        }
        else if (action == "listRooms") {
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

    nlohmann::json RoomController::handleCreateRoom(json& request) {
        json response = roomService_->createRoom(request);
        return response.dump();
    }

    nlohmann::json RoomController::handleJoinRoom(json& request) {
        json response = roomService_->joinRoom(request);
        return response.dump();
    }

    nlohmann::json RoomController::handleExitRoom(json& request) {
        json response = roomService_->exitRoom(request);
        return response.dump();
    }

    nlohmann::json RoomController::handleListRooms(json& request) {
        auto response = roomService_->listRooms();
        return response.dump();
    }

} // namespace game_server