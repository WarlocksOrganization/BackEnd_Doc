// repository/room_repository.cpp
// �� �������丮 ���� ����
// �� ���� �����ͺ��̽� �۾��� ó���ϴ� �������丮
#include "room_repository.h"
#include "../util/db_pool.h"
#include <pqxx/pqxx>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

namespace game_server {

    using json = nlohmann::json;

    // �������丮 ����ü
    class RoomRepositoryImpl : public RoomRepository {
    public:
        explicit RoomRepositoryImpl(DbPool* dbPool) : dbPool_(dbPool) {}

        std::vector<json> findAllOpen() override {
            std::vector<json> rooms;
            auto conn = dbPool_->get_connection();
            pqxx::work txn(*conn);
            try {
                // ���� �� ��� ��ȸ (�ֱ� ������)
                pqxx::result result = txn.exec_params(
                    "SELECT room_id, room_name, host_id, ip_address, port, "
                    "max_players, status, created_at "
                    "FROM rooms WHERE status = 'WAITING' "
                    "ORDER BY created_at DESC");

                txn.commit();
                dbPool_->return_connection(conn);

                // ��ȸ ����� Room ��ü ����Ʈ�� ��ȯ
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

        json createRoomWithHost(int hostId, const std::string& roomName, int maxPlayers) {
            auto conn = dbPool_->get_connection();
            pqxx::work txn(*conn);
            int roomId = -1;
            json result = {
                {"roomId", -1}
            };
            try {
                // ��ȿ�� �� ID ã��
                pqxx::result idResult = txn.exec(
                    "SELECT room_id FROM rooms WHERE status = 'TERMINATED' ORDER BY room_id LIMIT 1");

                if (idResult.empty()) {
                    txn.abort();
                    dbPool_->return_connection(conn);
                    return result;
                }
                roomId = idResult[0][0].as<int>();

                // �� ��Ȱ��ȭ
                pqxx::result roomResult;
                roomResult = txn.exec_params(
                    "UPDATE rooms SET room_name = $1, host_id = $2, max_players = $3, "
                    "status = 'WAITING', created_at = DEFAULT "
                    "WHERE room_id = $4 AND status = 'TERMINATED' "
                    "RETURNING room_id, ip_address, port",
                    roomName, hostId, maxPlayers, roomId);

                if (roomResult.empty()) {
                    txn.abort();
                    dbPool_->return_connection(conn);
                    return result;
                }

                // ����ڸ� �濡 �߰�
                txn.exec_params(
                    "INSERT INTO room_users(room_id, user_id) VALUES($1, $2)",
                    roomId, hostId);

                txn.commit();
                dbPool_->return_connection(conn);
                result["roomId"] = roomResult[0]["room_id"].as<int>();
                result["ipAddress"] = roomResult[0]["ip_address"].as<std::string>();
                result["port"] = roomResult[0]["port"].as<int>();
                return result;
            }
            catch (const std::exception& e) {
                spdlog::error("Error in createRoomWithHost: {}", e.what());
                txn.abort();
                dbPool_->return_connection(conn);
                return result;
            }
        }

        bool addPlayer(int roomId, int userId) override {
            auto conn = dbPool_->get_connection();
            pqxx::work txn(*conn);
            try {
                // ���� �����ϰ� WAITING �������� Ȯ��
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

                // �̹� ������ ��������� Ȯ��
                pqxx::result checkResult = txn.exec_params(
                    "SELECT joined_at FROM room_users "
                    "WHERE room_id = $1 AND user_id = $2",
                    roomId, userId);

                if (!checkResult.empty()) {
                    // �̹� ������ ����
                    spdlog::error("User {} already exists in room {}", userId, roomId);
                    txn.abort();
                    dbPool_->return_connection(conn);
                    return false;
                }

                // �ִ� �ο� Ȯ��
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

                // �� ������ �߰�
                pqxx::result result = txn.exec_params(
                    "INSERT INTO room_users (room_id, user_id, joined_at) "
                    "VALUES ($1, $2, DEFAULT) RETURNING room_id",
                    roomId, userId);

                if (result.empty()) {
                    txn.abort();
                    dbPool_->return_connection(conn);
                    return false;
                }

                txn.commit();
                dbPool_->return_connection(conn);
                spdlog::info("User {} joined room {}", userId, roomId);
                return true;
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
                // ����ڰ� ���� �� ID ��������
                pqxx::result roomResult = txn.exec_params(
                    "SELECT room_id FROM room_users WHERE user_id = $1",
                    userId);

                if (roomResult.empty()) {
                    // ����ڰ� � �濡�� ����
                    txn.abort();
                    dbPool_->return_connection(conn);
                    spdlog::warn("User {} is not in any room", userId);
                    return false;
                }

                int room_id = roomResult[0][0].as<int>();

                // ������ ����
                txn.exec_params(
                    "DELETE FROM room_users WHERE user_id = $1",
                    userId);

                // ���� Ʈ����� ������ �÷��̾� �� Ȯ��
                pqxx::result countResult = txn.exec_params(
                    "SELECT COUNT(*) FROM room_users WHERE room_id = $1",
                    room_id);

                int remaining_players = countResult[0][0].as<int>();

                // �濡 ���� �÷��̾ ������ �� ���� TERMINATED�� ����
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
                // ���� �÷��̾� �� Ȯ��
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
                // ���� �濡 �ִ� ������ ID ��� ��ȸ
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

    // ���丮 �޼��� ����
    std::unique_ptr<RoomRepository> RoomRepository::create(DbPool* dbPool) {
        return std::make_unique<RoomRepositoryImpl>(dbPool);
    }

} // namespace game_server