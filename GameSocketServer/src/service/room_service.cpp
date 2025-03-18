//// service/room_service.cpp
//// �� ���� ���� ����
//// �� ����, ����, ��� ��ȸ ���� ����Ͻ� ������ ó��
//#include "room_service.h"
//#include "../repository/room_repository.h"
//#include <spdlog/spdlog.h>
//#include <random>
//#include <string>
//#include <nlohmann/json.hpp>
//
//namespace game_server {
//
//    using json = nlohmann::json;
//
//    namespace {
//        // �� �̸� ��ȿ�� ���� �Լ�
//        bool isValidRoomName(const std::string& name) {
//            // �� �̸��� ��ȿ���� ����
//            if (name.empty()) {
//                return false;
//            }
//
//            // 40����Ʈ(UTF-8 ǥ��) �̳����� Ȯ��
//            if (name.size() > 40) {
//                return false;
//            }
//
//            // ����, �ѱ�, ���ڸ� �����ϴ��� Ȯ��
//            for (unsigned char c : name) {
//                // ASCII ����� ���� Ȯ��
//                if ((c >= 'A' && c <= 'Z') ||
//                    (c >= 'a' && c <= 'z') ||
//                    (c >= '0' && c <= '9') ||
//                    (c == ' ')) {
//                    continue;
//                }
//
//                // UTF-8 �ѱ� ���� Ȯ�� (ù ����Ʈ�� 0xEA~0xED ����)
//                if ((c & 0xF0) == 0xE0) {
//                    // �ѱ� ������ ù ����Ʈ ���ɼ�, �� �� ��Ȯ�� Ȯ�� �ʿ�
//                    continue;
//                }
//
//                // �ѱ� ������ ���� ����Ʈ (0x80~0xBF ����)
//                if ((c & 0xC0) == 0x80) {
//                    continue;
//                }
//
//                // ������ �ʴ� ����
//                return false;
//            }
//
//            return true;
//        }
//    }
//
//    // ���� ����ü
//    class RoomServiceImpl : public RoomService {
//    public:
//        explicit RoomServiceImpl(std::shared_ptr<RoomRepository> roomRepo)
//            : roomRepo_(roomRepo) {
//        }
//
//        json createRoom(const json& request) override {
//            json response;
//
//            // ��û ��ȿ�� ����
//            if (!isValidRoomName(request["room_name"])) {
//                response["stats"] = "error";
//                response["message"] = "Room name must be 1-40 bytes long and contain only English, Korean, or numbers";
//                return response;
//            }
//
//            if (request["max_players"] < 2 || request["max_players"] > 6) {
//                response["stats"] = "error";
//                response["message"] = "Max players must be between 2 and 6";
//                return response;
//            }
//
//            //// ���� �̸��� ���� �̹� �����ϴ��� Ȯ��
//            //if (roomRepo_->findByName(request.roomName)) {
//            //    response.success = false;
//            //    response.message = "A room with this name already exists";
//            //    return response;
//            //}
//
//            // �� ����
//            int roomId = roomRepo_->create(request["room_name"], request["room_id"], request["max_players"]);
//            if (roomId <= 0) {
//                response["stats"] = "error";
//                response["message"] = "Failed to create room";
//                return response;
//            }
//
//            // �� �����ڸ� �濡 �߰�
//            if (!roomRepo_->addPlayer(roomId, request["room_id"])) {
//                response["stats"] = "error";
//                response["message"] = "Failed to add room creator to room";
//                return response;
//            }
//
//            // ���� ���� ����
//            response["stats"] = "success";
//            response["message"] = "Room successfully created";
//            response["room_id"] = roomId;
//            response["room_name"] = request["room_name"];
//
//            spdlog::info("User {} created new room: {} (ID: {})",
//                request["user_id"], request["room_name"], roomId);
//
//            return response;
//        }
//
//        json joinRoom(const json& request) override {
//            json response;
//
//            // ��û ��ȿ�� ����
//            if (!request.contains("room_id")) {
//                response["status"] = "error";
//                response["message"] = "Missing room_id in request";
//                return response;
//            }
//
//            // �� ã��
//            auto room = roomRepo_->findById(request["room_id"]);
//            if (room["room_id"] == -1) {
//                response["status"] = "error";
//                response["message"] = "Room not found";
//                return response;
//            }
//
//            // �� ���� Ȯ��
//            if (room["status"] != "WAITING") {
//                response["status"] = "error";
//                response["message"] = "Room is not currently available";
//                return response;
//            }
//
//            // ���� ������ �� Ȯ��
//            int currentPlayers = roomRepo_->getPlayerCount(room["room_id"]);
//            if (currentPlayers >= room["max_players"]) {
//                response["status"] = "error";
//                response["message"] = "Room is full";
//                return response;
//            }
//
//            // ����ڸ� �濡 �߰�
//            if (!roomRepo_->addPlayer(room["room_id"], request["user_id"])) {
//                response["status"] = "error";
//                response["message"] = "Failed to join room";
//                return response;
//            }
//
//            // �� ������ ��� ��������
//            auto players = roomRepo_->getPlayersInRoom(room["room_id"]);
//
//            // ���� ���� ����
//            response["status"] = "success";
//            response["message"] = "Successfully joined room";
//            response["room_id"] = room["room_id"];
//            response["room_name"] = room["room_name"];
//            response["current_players"] = currentPlayers + 1; // ���� ������ ����� ����
//            response["max_players"] = room["max_players"];
//            response["players"] = players;
//
//            spdlog::info("User {} joined room {}", request["user_id"], room["room_id"]);
//            spdlog::info("Room {} current players: {}/{}",
//                room["room_id"], currentPlayers + 1, room["max_players"]);
//
//            return response;
//        }
//
//        json exitRoom(const json& request) override {
//            json response;
//
//            // ��û ��ȿ�� ����
//            if (!request.contains("user_id")) {
//                response["status"] = "error";
//                response["message"] = "Missing user_id in request";
//                return response;
//            }
//
//            // �� ã�� �� ����� ����
//            int roomId = roomRepo_->removePlayer(request["user_id"]);
//            if (roomId == -1) {
//                response["status"] = "error";
//                response["message"] = "User not found in any room";
//                return response;
//            }
//
//            // ���� ���� ����
//            response["status"] = "success";
//            response["message"] = "Successfully exited room";
//            response["room_id"] = roomId;
//
//            spdlog::info("User {} exited room {}", request["user_id"], roomId);
//
//            return response;
//        }
//
//        json listRooms(const json& request) override {
//            json response;
//
//            // ���� �� ��� ��������
//            auto rooms = roomRepo_->findAllOpen();
//
//            // ���� ����
//            response["status"] = "success";
//            response["message"] = "Successfully retrieved room list";
//            response["rooms"] = json::array();
//
//            for (const auto& room : rooms) {
//                json roomInfo;
//                roomInfo["room_id"] = room["room_id"];
//                roomInfo["room_name"] = room["room_name"];
//                roomInfo["host_id"] = room["host_id"];
//                roomInfo["current_players"] = roomRepo_->getPlayerCount(room["room_id"]);
//                roomInfo["max_players"] = room["max_players"];
//                roomInfo["status"] = room["status"];
//                roomInfo["created_at"] = room["created_at"];
//
//                response["rooms"].push_back(roomInfo);
//            }
//
//            spdlog::info("Retrieved {} open rooms", response["rooms"].size());
//
//            return response;
//        }
//
//    private:
//        std::shared_ptr<RoomRepository> roomRepo_;
//    };
//
//    // ���丮 �޼��� ����
//    std::unique_ptr<RoomService> RoomService::create(std::shared_ptr<RoomRepository> roomRepo) {
//        return std::make_unique<RoomServiceImpl>(roomRepo);
//    }
//
//} // namespace game_server