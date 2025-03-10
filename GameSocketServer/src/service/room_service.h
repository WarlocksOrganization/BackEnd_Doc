// service/room_service.h
#pragma once
#include "../repository/room_repository.h"
#include "../dto/request/create_room_request.h"
#include "../dto/request/join_room_request.h"
#include "../dto/response/create_room_response.h"
#include "../dto/response/join_room_response.h"
#include "../dto/response/list_rooms_response.h"
#include <memory>
#include <string>
#include <vector>

namespace game_server {

    class RoomService {
    public:
        virtual ~RoomService() = default;

        virtual CreateRoomResponse createRoom(const CreateRoomRequest& request, int userId) = 0;
        virtual JoinRoomResponse joinRoom(const JoinRoomRequest& request, int userId) = 0;
        virtual ListRoomsResponse listRooms() = 0;

        static std::unique_ptr<RoomService> create(std::shared_ptr<RoomRepository> roomRepo);
    };

} // namespace game_server