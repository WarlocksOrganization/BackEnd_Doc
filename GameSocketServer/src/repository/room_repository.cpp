// repository/room_repository.cpp
// 방 리포지토리 구현 파일
// 방 관련 데이터베이스 작업을 처리하는 리포지토리
#include "room_repository.h"
#include "../util/db_pool.h"
#include <pqxx/pqxx>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

namespace game_server {

    using json = nlohmann::json;

    // 리포지토리 구현체
    class RoomRepositoryImpl : public RoomRepository {
    public:
        explicit RoomRepositoryImpl(DbPool* dbPool) : dbPool_(dbPool) {}

        int findValidRoom() override {
            auto conn = dbPool_->get_connection();
            pqxx::work txn(*conn);
            try {
                // ID로 방 정보 조회 (가장 낮은 ID의 TERMINATED 상태 방을 찾음)
                pqxx::result result = txn.exec_params(
                    "SELECT room_id "
                    "FROM rooms WHERE status = 'TERMINATED' "
                    "ORDER BY room_id LIMIT 1");

                txn.commit();
                dbPool_->return_connection(conn);

                return result.empty() ? -1 : result[0][0].as<int>();
            }
            catch (const std::exception& e) {
                txn.abort();
                spdlog::error("Cannot find valid room: {}", e.what());
                dbPool_->return_connection(conn);
                return -1;
            }
        }

        std::vector<json> findAllOpen() override {
            std::vector<json> rooms;
            auto conn = dbPool_->get_connection();
            pqxx::work txn(*conn);
            try {
                // 열린 방 목록 조회 (최근 생성순)
                pqxx::result result = txn.exec_params(
                    "SELECT room_id, room_name, host_id, ip_address, port, "
                    "max_players, status, created_at "
                    "FROM rooms WHERE status = 'WAITING' "
                    "ORDER BY created_at DESC");

                txn.commit();
                dbPool_->return_connection(conn);

                // 조회 결과를 Room 객체 리스트로 변환
                for (const auto& row : result) {
                    json room;
                    room["roomId"] = row["room_id"].as<int>();
                    room["roomName"] = row["room_name"].as<std::string>();
                    room["hostId"] = row["host_id"].as<int>();
                    room["ipAddress"] = row["ip_address"].as<std::string>();
                    room["port"] = row["port"].as<int>();
                    room["maxPlayers"] = row["max_players"].as<int>();
                    room["status"] = row["status"].as<std::string>();
                    room["createdAt"] = row["created_at"].as<std::string>();
                    rooms.push_back(room);
                }
                return rooms;
            }
            catch (const std::exception& e) {
                spdlog::error("Error retrieving open rooms list: {}", e.what());
                txn.abort();
                dbPool_->return_connection(conn);
                return rooms;
            }
        }

        bool create(int hostId, int roomId, const std::string& roomName, int maxPlayers) override {
            auto conn = dbPool_->get_connection();
            pqxx::work txn(*conn);
            try {
                // 기존 TERMINATED 방 재활성화 시도
                pqxx::result result = txn.exec_params(
                    "UPDATE rooms "
                    "SET room_name = $1, host_id = $2, max_players = $3, status = 'WAITING', created_at = DEFAULT "
                    "WHERE room_id = $4 AND status = 'TERMINATED' "
                    "RETURNING room_id",
                    roomName, hostId, maxPlayers, roomId);

                if (result.empty()) {
                    spdlog::warn("Failed to reactivate room");
                    txn.abort();
                    dbPool_->return_connection(conn);
                    return false;
                }
                spdlog::info("Room {} reactivated with name '{}', host {}",
                    roomId, roomName, hostId);

                txn.commit();
                dbPool_->return_connection(conn);
                return true;
            }
            catch (const std::exception& e) {
                txn.abort();
                spdlog::error("Error creating room: {}", e.what());
                dbPool_->return_connection(conn);
                return false;
            }
        }

        bool addPlayer(int roomId, int userId) override {
            auto conn = dbPool_->get_connection();
            pqxx::work txn(*conn);
            try {
                // 방이 존재하고 WAITING 상태인지 확인
                pqxx::result roomCheck = txn.exec_params(
                    "SELECT status FROM rooms WHERE room_id = $1",
                    roomId);

                if (roomCheck.empty()) {
                    spdlog::error("Room {} does not exist", roomId);
                    txn.abort();
                    dbPool_->return_connection(conn);
                    return false;
                }

                std::string status = roomCheck[0][0].as<std::string>();
                if (status != "WAITING") {
                    spdlog::error("Cannot join room {} - status is {}", roomId, status);
                    txn.abort();
                    dbPool_->return_connection(conn);
                    return false;
                }

                // 이미 참가한 사용자인지 확인
                pqxx::result checkResult = txn.exec_params(
                    "SELECT joined_at FROM room_users "
                    "WHERE room_id = $1 AND user_id = $2",
                    roomId, userId);

                if (!checkResult.empty()) {
                    // 이미 참가한 상태
                    spdlog::error("User {} already exists in room {}", userId, roomId);
                    txn.abort();
                    dbPool_->return_connection(conn);
                    return false;
                }

                // 최대 인원 확인
                pqxx::result maxPlayersResult = txn.exec_params(
                    "SELECT max_players, "
                    "(SELECT COUNT(*) FROM room_users WHERE room_id = $1) as current_players "
                    "FROM rooms WHERE room_id = $1",
                    roomId);

                int maxPlayers = maxPlayersResult[0]["max_players"].as<int>();
                int currentPlayers = maxPlayersResult[0]["current_players"].as<int>();

                if (currentPlayers >= maxPlayers) {
                    spdlog::error("Room {} is full ({}/{})", roomId, currentPlayers, maxPlayers);
                    txn.abort();
                    dbPool_->return_connection(conn);
                    return false;
                }

                // 새 참가자 추가
                pqxx::result result = txn.exec_params(
                    "INSERT INTO room_users (room_id, user_id, joined_at) "
                    "VALUES ($1, $2, DEFAULT) RETURNING room_id",
                    roomId, userId);

                txn.commit();
                dbPool_->return_connection(conn);

                if (!result.empty()) {
                    spdlog::info("User {} joined room {}", userId, roomId);
                    return true;
                }
                return false;
            }
            catch (const std::exception& e) {
                spdlog::error("Error adding player to room: {}", e.what());
                txn.abort();
                dbPool_->return_connection(conn);
                return false;
            }
        }

        bool removePlayer(int userId) override {
            auto conn = dbPool_->get_connection();
            pqxx::work txn(*conn);
            try {
                // 사용자가 속한 방 ID 가져오기
                pqxx::result roomResult = txn.exec_params(
                    "SELECT room_id FROM room_users WHERE user_id = $1",
                    userId);

                if (roomResult.empty()) {
                    // 사용자가 어떤 방에도 없음
                    txn.commit();
                    dbPool_->return_connection(conn);
                    spdlog::warn("User {} is not in any room", userId);
                    return false;
                }

                int room_id = roomResult[0][0].as<int>();

                // 참가자 제거
                txn.exec_params(
                    "DELETE FROM room_users WHERE user_id = $1",
                    userId);

                // 동일 트랜잭션 내에서 플레이어 수 확인
                pqxx::result countResult = txn.exec_params(
                    "SELECT COUNT(*) FROM room_users WHERE room_id = $1",
                    room_id);

                int remaining_players = countResult[0][0].as<int>();

                // 방에 남은 플레이어가 없으면 방 상태 TERMINATED로 변경
                if (remaining_players == 0) {
                    txn.exec_params(
                        "UPDATE rooms SET status = 'TERMINATED' WHERE room_id = $1",
                        room_id);
                    spdlog::info("Room {} marked as TERMINATED (no players left)", room_id);
                }

                txn.commit();
                dbPool_->return_connection(conn);
                spdlog::info("User {} left room {}, {} players remaining",
                    userId, room_id, remaining_players);
                return true;
            }
            catch (const std::exception& e) {
                spdlog::error("Error removing player from room: {}", e.what());
                txn.abort();
                dbPool_->return_connection(conn);
                return false;
            }
        }

        int getPlayerCount(int roomId) override {
            auto conn = dbPool_->get_connection();
            pqxx::work txn(*conn);
            try {
                // 남은 플레이어 수 확인
                pqxx::result result = txn.exec_params(
                    "SELECT COUNT(*) FROM room_users WHERE room_id = $1",
                    roomId);

                txn.commit();
                dbPool_->return_connection(conn);

                return result.empty() ? 0 : result[0][0].as<int>();
            }
            catch (const std::exception& e) {
                spdlog::error("Error retrieving room player count: {}", e.what());
                txn.abort();
                dbPool_->return_connection(conn);
                return -1;
            }
        }

        std::vector<int> getPlayersInRoom(int roomId) override {
            std::vector<int> playerIds;
            auto conn = dbPool_->get_connection();
            pqxx::work txn(*conn);

            try {
                // 현재 방에 있는 참가자 ID 목록 조회
                pqxx::result result = txn.exec_params(
                    "SELECT user_id FROM room_users "
                    "WHERE room_id = $1",
                    roomId);

                txn.commit();
                dbPool_->return_connection(conn);

                for (const auto& row : result) {
                    playerIds.push_back(row[0].as<int>());
                }

                spdlog::debug("Found {} players in room {}", playerIds.size(), roomId);
            }
            catch (const std::exception& e) {
                spdlog::error("Error retrieving room player list: {}", e.what());
                txn.abort();
                dbPool_->return_connection(conn);
            }

            return playerIds;
        }

    private:
        DbPool* dbPool_;
    };

    // 팩토리 메서드 구현
    std::unique_ptr<RoomRepository> RoomRepository::create(DbPool* dbPool) {
        return std::make_unique<RoomRepositoryImpl>(dbPool);
    }

} // namespace game_server