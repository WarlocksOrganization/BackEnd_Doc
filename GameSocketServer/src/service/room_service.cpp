#include "room_service.h"
#include "../repository/room_repository.h"
#include <spdlog/spdlog.h>
#include <random>
#include <string>
#include <nlohmann/json.hpp>

namespace game_server {

    using json = nlohmann::json;

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

        json createRoom(json& request) override {
            json response;

            try {
                // ��û ��ȿ�� ����
                if (!request.contains("room_name") || !request.contains("user_id") || !request.contains("max_players")) {
                    response["status"] = "error";
                    response["message"] = "Missing required fields in request";
                    return response;
                }

                if (!isValidRoomName(request["room_name"])) {
                    response["status"] = "error";
                    response["message"] = "Room name must be 1-40 bytes long and contain only English, Korean, or numbers";
                    return response;
                }

                if (request["max_players"] < 2 || request["max_players"] > 8) {
                    response["status"] = "error";
                    response["message"] = "Max players must be between 2 and 8";
                    return response;
                }

                // �� ID ��� (���� ������ ���� ������ �� ID, ������ -1)
                int roomId = roomRepo_->findValidRoom();

                // �� ����
                if (!roomRepo_->create(request["user_id"], roomId, request["room_name"], request["max_players"])) {
                    response["status"] = "error";
                    response["message"] = "Failed to create room";
                    return response;
                }

                // �� �����ڸ� �濡 �߰�
                if (!roomRepo_->addPlayer(roomId, request["user_id"])) {
                    response["status"] = "error";
                    response["message"] = "Failed to add room creator to room";
                    return response;
                }

                // ���� ���� ����
                response["status"] = "success";
                response["message"] = "Room successfully created";
                response["room_id"] = roomId;
                response["room_name"] = request["room_name"];

                spdlog::info("User {} created new room: {} (ID: {})",
                    request["user_id"].get<int>(), request["room_name"].get<std::string>(), roomId);
            }
            catch (const std::exception& e) {
                response["status"] = "error";
                response["message"] = std::string("Error creating room: ") + e.what();
                spdlog::error("Error in createRoom: {}", e.what());
            }

            return response;
        }

        json joinRoom(json& request) override {
            json response;

            try {
                // ��û ��ȿ�� ����
                if (!request.contains("room_id") || !request.contains("user_id")) {
                    response["status"] = "error";
                    response["message"] = "Missing required fields in request";
                    return response;
                }

                int roomId = request["room_id"];
                int userId = request["user_id"];

                // �濡 ������ �߰�
                if (!roomRepo_->addPlayer(roomId, userId)) {
                    response["status"] = "error";
                    response["message"] = "Failed to join room - room may be full or not in WAITING state";
                    return response;
                }

                // ���� ���� ������ �� ��������
                int currentPlayers = roomRepo_->getPlayerCount(roomId);

                // �� ������ ��� ��������
                auto players = roomRepo_->getPlayersInRoom(roomId);

                // ���� ���� ����
                response["status"] = "success";
                response["message"] = "Successfully joined room";
                response["room_id"] = roomId;
                response["current_players"] = currentPlayers;
                response["players"] = players;

                spdlog::info("User {} joined room {}", userId, roomId);
                spdlog::info("Room {} current players: {}", roomId, currentPlayers);
            }
            catch (const std::exception& e) {
                response["status"] = "error";
                response["message"] = std::string("Error joining room: ") + e.what();
                spdlog::error("Error in joinRoom: {}", e.what());
            }

            return response;
        }

        json exitRoom(json& request) override {
            json response;

            try {
                // ��û ��ȿ�� ����
                if (!request.contains("user_id")) {
                    response["status"] = "error";
                    response["message"] = "Missing user_id in request";
                    return response;
                }

                int userId = request["user_id"];

                // �÷��̾ �濡�� ���� 
                if (!roomRepo_->removePlayer(userId)) {
                    response["status"] = "error";
                    response["message"] = "User not found in any room";
                    return response;
                }

                // ���� ���� ����
                response["status"] = "success";
                response["message"] = "Successfully exited room";

                spdlog::info("User {} exited room", userId);
            }
            catch (const std::exception& e) {
                response["status"] = "error";
                response["message"] = std::string("Error exiting room: ") + e.what();
                spdlog::error("Error in exitRoom: {}", e.what());
            }

            return response;
        }

        json listRooms() override {
            json response;

            try {
                // ���� �� ��� ��������
                auto rooms = roomRepo_->findAllOpen();

                // ���� ����
                response["status"] = "success";
                response["message"] = "Successfully retrieved room list";
                response["rooms"] = json::array();

                for (const auto& room : rooms) {
                    json roomInfo;
                    roomInfo["room_id"] = room["room_id"];
                    roomInfo["room_name"] = room["room_name"];
                    roomInfo["host_id"] = room["host_id"];
                    roomInfo["current_players"] = roomRepo_->getPlayerCount(room["room_id"]);
                    roomInfo["max_players"] = room["max_players"];
                    roomInfo["status"] = room["status"];
                    roomInfo["created_at"] = room["created_at"];

                    response["rooms"].push_back(roomInfo);
                }

                spdlog::info("Retrieved {} open rooms", response["rooms"].size());
            }
            catch (const std::exception& e) {
                response["status"] = "error";
                response["message"] = std::string("Error listing rooms: ") + e.what();
                spdlog::error("Error in listRooms: {}", e.what());
            }

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