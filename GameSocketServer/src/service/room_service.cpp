// service/room_service.cpp
// 방 서비스 구현 파일
// 방 생성, 참가, 목록 조회 등의 비즈니스 로직을 처리
#include "room_service.h"
#include "../repository/room_repository.h"
#include <spdlog/spdlog.h>
#include <random>
#include <string>

namespace game_server {

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

        CreateRoomResponse createRoom(const CreateRoomRequest& request, int userId) override {
            CreateRoomResponse response;

            // 요청 유효성 검증
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

            // 같은 이름의 방이 이미 존재하는지 확인
            if (roomRepo_->findByName(request.roomName)) {
                response.success = false;
                response.message = "A room with this name already exists";
                return response;
            }

            // 방 생성
            int roomId = roomRepo_->create(request.roomName, userId, request.maxPlayers, request.gameMode);
            if (roomId <= 0) {
                response.success = false;
                response.message = "Failed to create room";
                return response;
            }

            // 방 생성자를 방에 추가
            if (!roomRepo_->addPlayer(roomId, userId)) {
                response.success = false;
                response.message = "Failed to add room creator to room";
                return response;
            }

            // 성공 응답 생성
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

            // 방 찾기
            auto room = roomRepo_->findById(request.roomId);
            if (!room) {
                response.success = false;
                response.message = "Room not found";
                return response;
            }

            // 방 상태 확인
            if (room->status != "open") {
                response.success = false;
                response.message = "Room is not currently available";
                return response;
            }

            // 현재 참가자 수 확인
            int currentPlayers = roomRepo_->getPlayerCount(room->roomId);
            if (currentPlayers >= room->maxPlayers) {
                response.success = false;
                response.message = "Room is full";
                return response;
            }

            // 사용자를 방에 추가
            if (!roomRepo_->addPlayer(room->roomId, userId)) {
                response.success = false;
                response.message = "Failed to join room";
                return response;
            }

            // 방 참가자 목록 가져오기
            auto playerIds = roomRepo_->getPlayersInRoom(room->roomId);

            // 성공 응답 생성
            response.success = true;
            response.message = "Successfully joined room";
            response.roomId = room->roomId;
            response.roomName = room->roomName;
            response.currentPlayers = currentPlayers + 1; // 새로 참가한 사용자 포함
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

            // 열린 방 목록 가져오기
            auto rooms = roomRepo_->findAllOpen();

            // 응답 생성
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

    // 팩토리 메서드 구현
    std::unique_ptr<RoomService> RoomService::create(std::shared_ptr<RoomRepository> roomRepo) {
        return std::make_unique<RoomServiceImpl>(roomRepo);
    }

} // namespace game_server