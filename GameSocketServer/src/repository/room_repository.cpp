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

        int findValidRoom() override {
            auto conn = dbPool_->get_connection();
            try {
                pqxx::work txn(*conn);

                // ID�� �� ���� ��ȸ (���� ���� ID�� TERMINATED ���� ���� ã��)
                pqxx::result result = txn.exec_params(
                    "SELECT room_id "
                    "FROM rooms WHERE status = 'TERMINATED' "
                    "ORDER BY room_id LIMIT 1");

                txn.commit();
                dbPool_->return_connection(conn);

                return result.empty() ? -1 : result[0][0].as<int>();
            }
            catch (const std::exception& e) {
                spdlog::error("Cannot find valid room: {}", e.what());
                dbPool_->return_connection(conn);
                return -1;
            }
        }

        std::vector<json> findAllOpen() override {
            std::vector<json> rooms;
            auto conn = dbPool_->get_connection();
            try {
                pqxx::work txn(*conn);

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
                    room["room_id"] = row["room_id"].as<int>();
                    room["room_name"] = row["room_name"].as<std::string>();
                    room["host_id"] = row["host_id"].as<int>();
                    room["ip_address"] = row["ip_address"].as<std::string>();
                    room["port"] = row["port"].as<int>();
                    room["max_players"] = row["max_players"].as<int>();
                    room["status"] = row["status"].as<std::string>();
                    room["created_at"] = row["created_at"].as<std::string>();
                    rooms.push_back(room);
                }
                return rooms;
            }
            catch (const std::exception& e) {
                spdlog::error("Error retrieving open rooms list: {}", e.what());
                dbPool_->return_connection(conn);
                return rooms;
            }
        }

        bool create(int host_id, int room_id, const std::string& room_name, int max_players) override {
            auto conn = dbPool_->get_connection();
            try {
                pqxx::work txn(*conn);

                // ���� TERMINATED �� ��Ȱ��ȭ �õ�
                pqxx::result result = txn.exec_params(
                    "UPDATE rooms "
                    "SET room_name = $1, host_id = $2, max_players = $3, status = 'WAITING', created_at = DEFAULT "
                    "WHERE room_id = $4 AND status = 'TERMINATED' "
                    "RETURNING room_id",
                    room_name, host_id, max_players, room_id);

                if (result.empty()) {
                    spdlog::warn("Failed to reactivate room");
                    txn.abort();
                    dbPool_->return_connection(conn);
                    return false;
                }
                spdlog::info("Room {} reactivated with name '{}', host {}",
                    room_id, room_name, host_id);

                txn.commit();
                dbPool_->return_connection(conn);
                return true;
            }
            catch (const std::exception& e) {
                spdlog::error("Error creating room: {}", e.what());
                dbPool_->return_connection(conn);
                return false;
            }
        }

        bool addPlayer(int room_id, int user_id) override {
            auto conn = dbPool_->get_connection();
            try {
                pqxx::work txn(*conn);

                // ���� �����ϰ� WAITING �������� Ȯ��
                pqxx::result roomCheck = txn.exec_params(
                    "SELECT status FROM rooms WHERE room_id = $1",
                    room_id);

                if (roomCheck.empty()) {
                    spdlog::error("Room {} does not exist", room_id);
                    txn.abort();
                    dbPool_->return_connection(conn);
                    return false;
                }

                std::string status = roomCheck[0][0].as<std::string>();
                if (status != "WAITING") {
                    spdlog::error("Cannot join room {} - status is {}", room_id, status);
                    txn.abort();
                    dbPool_->return_connection(conn);
                    return false;
                }

                // �̹� ������ ��������� Ȯ��
                pqxx::result checkResult = txn.exec_params(
                    "SELECT joined_at FROM room_users "
                    "WHERE room_id = $1 AND user_id = $2",
                    room_id, user_id);

                if (!checkResult.empty()) {
                    // �̹� ������ ����
                    spdlog::error("User {} already exists in room {}", user_id, room_id);
                    txn.abort();
                    dbPool_->return_connection(conn);
                    return false;
                }

                // �ִ� �ο� Ȯ��
                pqxx::result maxPlayersResult = txn.exec_params(
                    "SELECT max_players, "
                    "(SELECT COUNT(*) FROM room_users WHERE room_id = $1) as current_players "
                    "FROM rooms WHERE room_id = $1",
                    room_id);

                int max_players = maxPlayersResult[0]["max_players"].as<int>();
                int current_players = maxPlayersResult[0]["current_players"].as<int>();

                if (current_players >= max_players) {
                    spdlog::error("Room {} is full ({}/{})", room_id, current_players, max_players);
                    txn.abort();
                    dbPool_->return_connection(conn);
                    return false;
                }

                // �� ������ �߰�
                pqxx::result result = txn.exec_params(
                    "INSERT INTO room_users (room_id, user_id, joined_at) "
                    "VALUES ($1, $2, DEFAULT) RETURNING room_id",
                    room_id, user_id);

                txn.commit();
                dbPool_->return_connection(conn);

                if (!result.empty()) {
                    spdlog::info("User {} joined room {}", user_id, room_id);
                    return true;
                }
                return false;
            }
            catch (const std::exception& e) {
                spdlog::error("Error adding player to room: {}", e.what());
                dbPool_->return_connection(conn);
                return false;
            }
        }

        bool removePlayer(int user_id) override {
            auto conn = dbPool_->get_connection();
            try {
                pqxx::work txn(*conn);

                // ����ڰ� ���� �� ID ��������
                pqxx::result roomResult = txn.exec_params(
                    "SELECT room_id FROM room_users WHERE user_id = $1",
                    user_id);

                if (roomResult.empty()) {
                    // ����ڰ� � �濡�� ����
                    txn.commit();
                    dbPool_->return_connection(conn);
                    spdlog::warn("User {} is not in any room", user_id);
                    return false;
                }

                int room_id = roomResult[0][0].as<int>();

                // ������ ����
                txn.exec_params(
                    "DELETE FROM room_users WHERE user_id = $1",
                    user_id);

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
                    user_id, room_id, remaining_players);
                return true;
            }
            catch (const std::exception& e) {
                spdlog::error("Error removing player from room: {}", e.what());
                dbPool_->return_connection(conn);
                return false;
            }
        }

        int getPlayerCount(int room_id) override {
            auto conn = dbPool_->get_connection();
            try {
                pqxx::work txn(*conn);

                // ���� �÷��̾� �� Ȯ��
                pqxx::result result = txn.exec_params(
                    "SELECT COUNT(*) FROM room_users WHERE room_id = $1",
                    room_id);

                txn.commit();
                dbPool_->return_connection(conn);

                return result.empty() ? 0 : result[0][0].as<int>();
            }
            catch (const std::exception& e) {
                spdlog::error("Error retrieving room player count: {}", e.what());
                dbPool_->return_connection(conn);
                return -1;
            }
        }

        std::vector<int> getPlayersInRoom(int room_id) override {
            std::vector<int> playerIds;
            auto conn = dbPool_->get_connection();

            try {
                pqxx::work txn(*conn);

                // ���� �濡 �ִ� ������ ID ��� ��ȸ
                pqxx::result result = txn.exec_params(
                    "SELECT user_id FROM room_users "
                    "WHERE room_id = $1",
                    room_id);

                txn.commit();
                dbPool_->return_connection(conn);

                for (const auto& row : result) {
                    playerIds.push_back(row[0].as<int>());
                }

                spdlog::debug("Found {} players in room {}", playerIds.size(), room_id);
            }
            catch (const std::exception& e) {
                spdlog::error("Error retrieving room player list: {}", e.what());
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