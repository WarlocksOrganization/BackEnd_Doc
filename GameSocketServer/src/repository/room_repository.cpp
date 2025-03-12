// repository/room_repository.cpp
// �� �������丮 ���� ����
// �� ���� �����ͺ��̽� �۾��� ó���ϴ� �������丮
#include "room_repository.h"
#include "../util/db_pool.h"
#include <pqxx/pqxx>
#include <spdlog/spdlog.h>

namespace game_server {

    // �������丮 ����ü
    class RoomRepositoryImpl : public RoomRepository {
    public:
        explicit RoomRepositoryImpl(DbPool* dbPool) : dbPool_(dbPool) {}

        std::optional<Room> findById(int roomId) override {
            auto conn = dbPool_->get_connection();
            try {
                pqxx::work txn(*conn);

                // ID�� �� ���� ��ȸ
                pqxx::result result = txn.exec_params(
                    "SELECT room_id, room_name, creator_id, created_at, closed_at, "
                    "max_players, map_id, status "
                    "FROM Rooms WHERE room_id = $1",
                    roomId);

                txn.commit();
                dbPool_->return_connection(conn);

                if (result.empty()) {
                    return std::nullopt;
                }

                // ��ȸ ����� Room ��ü ����
                Room room;
                room.roomId = result[0][0].as<int>();
                room.roomName = result[0][1].as<std::string>();
                room.creatorId = result[0][2].as<int>();
                room.createdAt = result[0][3].as<std::string>();

                if (!result[0][4].is_null()) {
                    room.closedAt = result[0][4].as<std::string>();
                }

                room.maxPlayers = result[0][5].as<int>();
                room.mapId = result[0][6].as<int>();
                room.status = result[0][8].as<std::string>();

                return room;
            }
            catch (const std::exception& e) {
                spdlog::error("Error finding room by ID: {}", e.what());
                dbPool_->return_connection(conn);
                return std::nullopt;
            }
        }

        std::optional<Room> findByName(const std::string& roomName) override {
            auto conn = dbPool_->get_connection();
            try {
                pqxx::work txn(*conn);

                // �̸����� �� ���� ��ȸ
                pqxx::result result = txn.exec_params(
                    "SELECT room_id, room_name, creator_id, created_at, closed_at, "
                    "max_players, map_id, status "
                    "FROM Rooms WHERE room_name = $1",
                    roomName);

                txn.commit();
                dbPool_->return_connection(conn);

                if (result.empty()) {
                    return std::nullopt;
                }

                // ��ȸ ����� Room ��ü ����
                Room room;
                room.roomId = result[0][0].as<int>();
                room.roomName = result[0][1].as<std::string>();
                room.creatorId = result[0][2].as<int>();
                room.createdAt = result[0][3].as<std::string>();

                if (!result[0][4].is_null()) {
                    room.closedAt = result[0][4].as<std::string>();
                }

                room.maxPlayers = result[0][5].as<int>();
                room.mapId = result[0][6].as<int>();
                room.status = result[0][8].as<std::string>();

                return room;
            }
            catch (const std::exception& e) {
                spdlog::error("Error finding room by name: {}", e.what());
                dbPool_->return_connection(conn);
                return std::nullopt;
            }
        }

        std::vector<Room> findAllOpen(int limit) override {
            std::vector<Room> rooms;
            auto conn = dbPool_->get_connection();
            try {
                pqxx::work txn(*conn);

                // ���� �� ��� ��ȸ (�ֱ� ������)
                pqxx::result result = txn.exec_params(
                    "SELECT room_id, room_name, creator_id, created_at, closed_at, "
                    "max_players, map_id, status "
                    "FROM Rooms WHERE status = 'open' "
                    "ORDER BY created_at DESC LIMIT $1",
                    limit);

                txn.commit();
                dbPool_->return_connection(conn);

                // ��ȸ ����� Room ��ü ����Ʈ�� ��ȯ
                for (const auto& row : result) {
                    Room room;
                    room.roomId = row[0].as<int>();
                    room.roomName = row[1].as<std::string>();
                    room.creatorId = row[2].as<int>();
                    room.createdAt = row[3].as<std::string>();

                    if (!row[4].is_null()) {
                        room.closedAt = row[4].as<std::string>();
                    }

                    room.maxPlayers = row[5].as<int>();
                    room.mapId = row[6].as<int>();
                    room.status = row[8].as<std::string>();

                    rooms.push_back(room);
                }
            }
            catch (const std::exception& e) {
                spdlog::error("Error retrieving open rooms list: {}", e.what());
                dbPool_->return_connection(conn);
            }

            return rooms;
        }

        int create(const std::string& roomName, int creatorId, int maxPlayers) override {
            auto conn = dbPool_->get_connection();
            try {
                pqxx::work txn(*conn);

                // �� �� ����
                pqxx::result result = txn.exec_params(
                    "INSERT INTO Rooms (room_name, creator_id, created_at, max_players, "
                    "game_mode, status) "
                    "VALUES ($1, $2, CURRENT_TIMESTAMP, $3, $4, 'open') RETURNING room_id",
                    roomName, creatorId, maxPlayers);

                txn.commit();
                dbPool_->return_connection(conn);

                if (result.empty()) {
                    return -1;
                }

                return result[0][0].as<int>();
            }
            catch (const std::exception& e) {
                spdlog::error("Error creating room: {}", e.what());
                dbPool_->return_connection(conn);
                return -1;
            }
        }

        bool addPlayer(int roomId, int userId) override {
            auto conn = dbPool_->get_connection();
            try {
                pqxx::work txn(*conn);

                // �̹� ������ ��������� Ȯ��
                pqxx::result checkResult = txn.exec_params(
                    "SELECT id FROM RoomPlayers "
                    "WHERE room_id = $1 AND user_id = $2 AND leave_time IS NULL",
                    roomId, userId);

                if (!checkResult.empty()) {
                    // �̹� ������ ����
                    txn.commit();
                    dbPool_->return_connection(conn);
                    return true;
                }

                // �� ������ �߰�
                pqxx::result result = txn.exec_params(
                    "INSERT INTO RoomPlayers (room_id, user_id, join_time) "
                    "VALUES ($1, $2, CURRENT_TIMESTAMP) RETURNING id",
                    roomId, userId);

                txn.commit();
                dbPool_->return_connection(conn);

                return !result.empty();
            }
            catch (const std::exception& e) {
                spdlog::error("Error processing player room join: {}", e.what());
                dbPool_->return_connection(conn);
                return false;
            }
        }

        bool removePlayer(int roomId, int userId) override {
            auto conn = dbPool_->get_connection();
            try {
                pqxx::work txn(*conn);

                // ������ ���� (leave_time ����)
                pqxx::result result = txn.exec_params(
                    "UPDATE RoomPlayers SET leave_time = CURRENT_TIMESTAMP "
                    "WHERE room_id = $1 AND user_id = $2 AND leave_time IS NULL "
                    "RETURNING id",
                    roomId, userId);

                txn.commit();
                dbPool_->return_connection(conn);

                return !result.empty();
            }
            catch (const std::exception& e) {
                spdlog::error("Error processing player room leave: {}", e.what());
                dbPool_->return_connection(conn);
                return false;
            }
        }

        int getPlayerCount(int roomId) override {
            auto conn = dbPool_->get_connection();
            try {
                pqxx::work txn(*conn);

                // ���� �濡 �ִ� ������ �� ��ȸ
                pqxx::result result = txn.exec_params(
                    "SELECT COUNT(*) FROM RoomPlayers "
                    "WHERE room_id = $1 AND leave_time IS NULL",
                    roomId);

                txn.commit();
                dbPool_->return_connection(conn);

                if (result.empty()) {
                    return 0;
                }

                return result[0][0].as<int>();
            }
            catch (const std::exception& e) {
                spdlog::error("Error retrieving room player count: {}", e.what());
                dbPool_->return_connection(conn);
                return 0;
            }
        }

        std::vector<int> getPlayersInRoom(int roomId) override {
            std::vector<int> playerIds;
            auto conn = dbPool_->get_connection();

            try {
                pqxx::work txn(*conn);

                // ���� �濡 �ִ� ������ ID ��� ��ȸ
                pqxx::result result = txn.exec_params(
                    "SELECT user_id FROM RoomPlayers "
                    "WHERE room_id = $1 AND leave_time IS NULL",
                    roomId);

                txn.commit();
                dbPool_->return_connection(conn);

                for (const auto& row : result) {
                    playerIds.push_back(row[0].as<int>());
                }
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