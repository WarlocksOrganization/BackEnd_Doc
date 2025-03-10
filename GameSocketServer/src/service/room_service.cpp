// service/room_service.cpp
#include "room_service.h"
#include "../repository/room_repository.h"
#include <spdlog/spdlog.h>
#include <random>
#include <string>

namespace game_server {

    namespace {
        // Validate room name function
        bool isValidRoomName(const std::string& name) {
            // Empty name is invalid
            if (name.empty()) {
                return false;
            }

            // Check if within 40 bytes (UTF-8 standard)
            if (name.size() > 40) {
                return false;
            }

            // Check if only contains English, Korean, numbers
            for (unsigned char c : name) {
                // Check ASCII English and numbers
                if ((c >= 'A' && c <= 'Z') ||
                    (c >= 'a' && c <= 'z') ||
                    (c >= '0' && c <= '9') ||
                    (c == ' ')) {
                    continue;
                }

                // Check UTF-8 Korean range (first byte in 0xEA~0xED range)
                if ((c & 0xF0) == 0xE0) {
                    // Possible first byte of Korean character, more precise check needed
                    continue;
                }

                // Possible continuation byte of Korean character (0x80~0xBF range)
                if ((c & 0xC0) == 0x80) {
                    continue;
                }

                // Disallowed character
                return false;
            }

            return true;
        }
    }

    // Service implementation
    class RoomServiceImpl : public RoomService {
    public:
        explicit RoomServiceImpl(std::shared_ptr<RoomRepository> roomRepo)
            : roomRepo_(roomRepo) {
        }

        CreateRoomResponse createRoom(const CreateRoomRequest& request, int userId) override {
            CreateRoomResponse response;

            // Validate request
            if (!isValidRoomName(request.roomName)) {
                response.success = false;
                response.message = "Room name must be 1-40 bytes long and contain only English, Korean, or numbers";
                return response;
            }

            if (request.maxPlayers < 2 || request.maxPlayers > 6) {
                response.success = false;
                response.message = "Max players must be between 2 and 6";
                return response;
            }

            // Check if room with same name already exists
            if (roomRepo_->findByName(request.roomName)) {
                response.success = false;
                response.message = "A room with this name already exists";
                return response;
            }

            // Create room
            int roomId = roomRepo_->create(request.roomName, userId, request.maxPlayers, request.gameMode);
            if (roomId <= 0) {
                response.success = false;
                response.message = "Failed to create room";
                return response;
            }

            // Add creator to room
            if (!roomRepo_->addPlayer(roomId, userId)) {
                response.success = false;
                response.message = "Failed to add room creator to room";
                return response;
            }

            // Create successful response
            response.success = true;
            response.message = "Room successfully created";
            response.roomId = roomId;
            response.roomName = request.roomName;

            spdlog::info("User {} created new room: {} (ID: {})",
                userId, request.roomName, roomId);

            return response;
        }

        JoinRoomResponse joinRoom(const JoinRoomRequest& request, int userId) override {
            JoinRoomResponse response;

            // Find room
            auto room = roomRepo_->findById(request.roomId);
            if (!room) {
                response.success = false;
                response.message = "Room not found";
                return response;
            }

            // Check room status
            if (room->status != "open") {
                response.success = false;
                response.message = "Room is not currently available";
                return response;
            }

            // Check current player count
            int currentPlayers = roomRepo_->getPlayerCount(room->roomId);
            if (currentPlayers >= room->maxPlayers) {
                response.success = false;
                response.message = "Room is full";
                return response;
            }

            // Add player to room
            if (!roomRepo_->addPlayer(room->roomId, userId)) {
                response.success = false;
                response.message = "Failed to join room";
                return response;
            }

            // Get room player list
            auto playerIds = roomRepo_->getPlayersInRoom(room->roomId);

            // Create successful response
            response.success = true;
            response.message = "Successfully joined room";
            response.roomId = room->roomId;
            response.roomName = room->roomName;
            response.currentPlayers = currentPlayers + 1; // Including newly joined player
            response.maxPlayers = room->maxPlayers;
            response.gameMode = room->gameMode;
            response.playerIds = playerIds;

            spdlog::info("User {} joined room {}",
                userId, room->roomId);
            spdlog::info("Room {} current players: {}/{}",
                room->roomId, response.currentPlayers, response.maxPlayers);

            return response;
        }

        ListRoomsResponse listRooms() override {
            ListRoomsResponse response;

            // Get open rooms list
            auto rooms = roomRepo_->findAllOpen();

            // Create response
            response.success = true;
            response.message = "Successfully retrieved room list";

            for (const auto& room : rooms) {
                int currentPlayers = roomRepo_->getPlayerCount(room.roomId);

                RoomInfo roomInfo;
                roomInfo.roomId = room.roomId;
                roomInfo.roomName = room.roomName;
                roomInfo.creatorId = room.creatorId;
                roomInfo.currentPlayers = currentPlayers;
                roomInfo.maxPlayers = room.maxPlayers;
                roomInfo.gameMode = room.gameMode;
                roomInfo.status = room.status;
                roomInfo.createdAt = room.createdAt;

                response.rooms.push_back(roomInfo);
            }

            spdlog::info("Retrieved {} open rooms", response.rooms.size());

            return response;
        }

    private:
        std::shared_ptr<RoomRepository> roomRepo_;
    };

    // Factory method implementation
    std::unique_ptr<RoomService> RoomService::create(std::shared_ptr<RoomRepository> roomRepo) {
        return std::make_unique<RoomServiceImpl>(roomRepo);
    }

} // namespace game_server