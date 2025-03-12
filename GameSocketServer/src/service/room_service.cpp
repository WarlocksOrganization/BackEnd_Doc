// service/room_service.cpp
// �� ���� ���� ����
// �� ����, ����, ��� ��ȸ ���� ����Ͻ� ������ ó��
#include "room_service.h"
#include "../repository/room_repository.h"
#include <spdlog/spdlog.h>
#include <random>
#include <string>

namespace game_server {

    namespace {
        // �� �̸� ��ȿ�� ���� �Լ�
        bool isValidRoomName(const std::string& name) {
            // �� �̸��� ��ȿ���� ����
            if (name.empty()) {
                return false;
            }

            // 40����Ʈ(UTF-8 ǥ��) �̳����� Ȯ��
            if (name.size() > 40) {
                return false;
            }

            // ����, �ѱ�, ���ڸ� �����ϴ��� Ȯ��
            for (unsigned char c : name) {
                // ASCII ����� ���� Ȯ��
                if ((c >= 'A' && c <= 'Z') ||
                    (c >= 'a' && c <= 'z') ||
                    (c >= '0' && c <= '9') ||
                    (c == ' ')) {
                    continue;
                }

                // UTF-8 �ѱ� ���� Ȯ�� (ù ����Ʈ�� 0xEA~0xED ����)
                if ((c & 0xF0) == 0xE0) {
                    // �ѱ� ������ ù ����Ʈ ���ɼ�, �� �� ��Ȯ�� Ȯ�� �ʿ�
                    continue;
                }

                // �ѱ� ������ ���� ����Ʈ (0x80~0xBF ����)
                if ((c & 0xC0) == 0x80) {
                    continue;
                }

                // ������ �ʴ� ����
                return false;
            }

            return true;
        }
    }

    // ���� ����ü
    class RoomServiceImpl : public RoomService {
    public:
        explicit RoomServiceImpl(std::shared_ptr<RoomRepository> roomRepo)
            : roomRepo_(roomRepo) {
        }

        CreateRoomResponse createRoom(const CreateRoomRequest& request, int userId) override {
            CreateRoomResponse response;

            // ��û ��ȿ�� ����
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

            // ���� �̸��� ���� �̹� �����ϴ��� Ȯ��
            if (roomRepo_->findByName(request.roomName)) {
                response.success = false;
                response.message = "A room with this name already exists";
                return response;
            }

            // �� ����
            int roomId = roomRepo_->create(request.roomName, userId, request.maxPlayers, request.gameMode);
            if (roomId <= 0) {
                response.success = false;
                response.message = "Failed to create room";
                return response;
            }

            // �� �����ڸ� �濡 �߰�
            if (!roomRepo_->addPlayer(roomId, userId)) {
                response.success = false;
                response.message = "Failed to add room creator to room";
                return response;
            }

            // ���� ���� ����
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

            // �� ã��
            auto room = roomRepo_->findById(request.roomId);
            if (!room) {
                response.success = false;
                response.message = "Room not found";
                return response;
            }

            // �� ���� Ȯ��
            if (room->status != "open") {
                response.success = false;
                response.message = "Room is not currently available";
                return response;
            }

            // ���� ������ �� Ȯ��
            int currentPlayers = roomRepo_->getPlayerCount(room->roomId);
            if (currentPlayers >= room->maxPlayers) {
                response.success = false;
                response.message = "Room is full";
                return response;
            }

            // ����ڸ� �濡 �߰�
            if (!roomRepo_->addPlayer(room->roomId, userId)) {
                response.success = false;
                response.message = "Failed to join room";
                return response;
            }

            // �� ������ ��� ��������
            auto playerIds = roomRepo_->getPlayersInRoom(room->roomId);

            // ���� ���� ����
            response.success = true;
            response.message = "Successfully joined room";
            response.roomId = room->roomId;
            response.roomName = room->roomName;
            response.currentPlayers = currentPlayers + 1; // ���� ������ ����� ����
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

            // ���� �� ��� ��������
            auto rooms = roomRepo_->findAllOpen();

            // ���� ����
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

    // ���丮 �޼��� ����
    std::unique_ptr<RoomService> RoomService::create(std::shared_ptr<RoomRepository> roomRepo) {
        return std::make_unique<RoomServiceImpl>(roomRepo);
    }

} // namespace game_server