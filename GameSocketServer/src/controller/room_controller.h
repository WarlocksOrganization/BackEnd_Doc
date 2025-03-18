// controller/room_controller.h
#pragma once
#include "controller.h"
#include "../service/room_service.h"
#include <memory>

namespace game_server {

    class RoomController : public Controller {
    public:
        explicit RoomController(std::shared_ptr<RoomService> roomService);
        ~RoomController() override = default;

        std::string handleRequest(nlohmann::json& request) override;

    private:
        std::string handleCreateRoom(nlohmann::json& request);
        std::string handleJoinRoom(nlohmann::json& request);
        std::string handleExitRoom(nlohmann::json& request);
        std::string handleListRooms(nlohmann::json& request);

        std::shared_ptr<RoomService> roomService_;
    };

} // namespace game_server