#include "room_service.h"
#include "../repository/room_repository.h"
#include <spdlog/spdlog.h>
#include <random>
#include <string>
#include <nlohmann/json.hpp>

namespace game_server {

    using json = nlohmann::json;

    namespace {
        // 방 이름 유효성 검증 함수
        bool isValidRoomName(const std::string& name) {
            // 빈 이름은 유효하지 않음
            if (name.empty()) {
                return false;
            }

            // 40바이트(UTF-8 표준) 이내인지 확인
            if (name.size() > 40) {
                return false;
            }

            // 영어, 한글, 숫자만 포함하는지 확인
            for (unsigned char c : name) {
                // ASCII 영어와 숫자 확인
                if ((c >= 'A' && c <= 'Z') ||
                    (c >= 'a' && c <= 'z') ||
                    (c >= '0' && c <= '9') ||
                    (c == ' ')) {
                    continue;
                }

                // UTF-8 한글 범위 확인 (첫 바이트가 0xEA~0xED 범위)
                if ((c & 0xF0) == 0xE0) {
                    // 한글 문자의 첫 바이트 가능성, 좀 더 정확한 확인 필요
                    continue;
                }

                // 한글 문자의 연속 바이트 (0x80~0xBF 범위)
                if ((c & 0xC0) == 0x80) {
                    continue;
                }

                // 허용되지 않는 문자
                return false;
            }

            return true;
        }
    }

    // 서비스 구현체
    class RoomServiceImpl : public RoomService {
    public:
        explicit RoomServiceImpl(std::shared_ptr<RoomRepository> roomRepo)
            : roomRepo_(roomRepo) {
        }

        json createRoom(json& request) override {
            json response;

            try {
                // 요청 유효성 검증
                if (!request.contains("roomName") || !request.contains("userId") || !request.contains("maxPlayers")) {
                    response["status"] = "error";
                    response["message"] = "Missing required fields in request";
                    return response;
                }

                if (!isValidRoomName(request["roomName"])) {
                    response["status"] = "error";
                    response["message"] = "Room name must be 1-40 bytes long and contain only English, Korean, or numbers";
                    return response;
                }

                if (request["maxPlayers"] < 2 || request["maxPlayers"] > 8) {
                    response["status"] = "error";
                    response["message"] = "Max players must be between 2 and 8";
                    return response;
                }

                // 단일 트랜잭션으로 방 생성 및 호스트 추가
                json result = roomRepo_->createRoomWithHost(
                    request["userId"], request["roomName"], request["maxPlayers"]);
                if (result["roomId"] == -1) {
                    response["status"] = "error";
                    response["message"] = "Failed to create room";
                    return response;
                }

                // 성공 응답 생성
                response["status"] = "success";
                response["message"] = "Room successfully created";
                response["roomId"] = result["roomId"];
                response["ipAddress"] = result["ipAddress"];
                response["port"] = result["port"];

                spdlog::info("User {} created new room: {} (ID: {})",
                    request["userId"].get<int>(), request["roomName"].get<std::string>(), result["roomId"].get<int>());
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
                // 요청 유효성 검증
                if (!request.contains("roomId") || !request.contains("userId")) {
                    response["status"] = "error";
                    response["message"] = "Missing required fields in request";
                    return response;
                }

                int roomId = request["roomId"];
                int userId = request["userId"];

                // 방에 참가자 추가
                if (!roomRepo_->addPlayer(roomId, userId)) {
                    response["status"] = "error";
                    response["message"] = "Failed to join room - room may be full or not in WAITING state";
                    return response;
                }

                // 성공 응답 생성
                response["status"] = "success";
                response["message"] = "Successfully joined room";

                spdlog::info("User {} joined room {}", userId, roomId);
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
                // 요청 유효성 검증
                if (!request.contains("userId")) {
                    response["status"] = "error";
                    response["message"] = "Missing userId in request";
                    return response;
                }

                int userId = request["userId"];

                // 플레이어를 방에서 제거 
                if (!roomRepo_->removePlayer(userId)) {
                    response["status"] = "error";
                    response["message"] = "User not found in any room";
                    return response;
                }

                // 성공 응답 생성
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
                // 열린 방 목록 가져오기
                auto rooms = roomRepo_->findAllOpen();

                // 응답 생성
                response["status"] = "success";
                response["message"] = "Successfully retrieved room list";
                response["rooms"] = json::array();

                for (const auto& room : rooms) {
                    json roomInfo;
                    roomInfo["roomId"] = room["roomId"];
                    roomInfo["roomName"] = room["roomName"];
                    roomInfo["hostId"] = room["hostId"];
                    roomInfo["currentPlayers"] = roomRepo_->getPlayerCount(room["roomId"]);
                    roomInfo["maxPlayers"] = room["maxPlayers"];
                    roomInfo["status"] = room["status"];
                    roomInfo["createdAt"] = room["createdAt"];

                    response["rooms"].push_back(roomInfo);
                }

                spdlog::info("Retrieved {} open room(s)", response["rooms"].size());
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

    // 팩토리 메서드 구현
    std::unique_ptr<RoomService> RoomService::create(std::shared_ptr<RoomRepository> roomRepo) {
        return std::make_unique<RoomServiceImpl>(roomRepo);
    }

} // namespace game_server