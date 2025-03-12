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

        std::string handleRequest(const nlohmann::json& request) override;

    private:
        std::string handleCreateRoom(const nlohmann::json& request);
        std::string handleJoinRoom(const nlohmann::json& request);
        std::string handleExitRoom(const nlohmann::json& request);
        std::string handleListRooms(const nlohmann::json& request);

        std::shared_ptr<RoomService> roomService_;
    };

} // namespace game_server