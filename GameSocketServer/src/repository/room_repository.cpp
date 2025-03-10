// repository/room_repository.cpp
#include "room_repository.h"
#include "../util/db_pool.h"
#include <pqxx/pqxx>
#include <spdlog/spdlog.h>

namespace game_server {

    // Repository implementation
    class RoomRepositoryImpl : public RoomRepository {
    public:
        explicit RoomRepositoryImpl(DbPool* dbPool) : dbPool_(dbPool) {}

        std::optional<Room> findById(int roomId) override {
            auto conn = dbPool_->get_connection();
            try {
                pqxx::work txn(*conn);

                pqxx::result result = txn.exec_params(
                    "SELECT room_id, room_name, creator_id, created_at, closed_at, "
                    "max_players, map_id, game_mode, status "
                    "FROM Rooms WHERE room_id = $1",
                    roomId);

                txn.commit();
                dbPool_->return_connection(conn);

                if (result.empty()) {
                    return std::nullopt;
                }

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
                room.gameMode = result[0][7].as<std::string>();
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

                pqxx::result result = txn.exec_params(
                    "SELECT room_id, room_name, creator_id, created_at, closed_at, "
                    "max_players, map_id, game_mode, status "
                    "FROM Rooms WHERE room_name = $1",
                    roomName);

                txn.commit();
                dbPool_->return_connection(conn);

                if (result.empty()) {
                    return std::nullopt;
                }

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
                room.gameMode = result[0][7].as<std::string>();
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

                pqxx::result result = txn.exec_params(
                    "SELECT room_id, room_name, creator_id, created_at, closed_at, "
                    "max_players, map_id, game_mode, status "
                    "FROM Rooms WHERE status = 'open' "
                    "ORDER BY created_at DESC LIMIT $1",
                    limit);

                txn.commit();
                dbPool_->return_connection(conn);

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
                    room.gameMode = row[7].as<std::string>();
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

        int create(const std::string& roomName, int creatorId, int maxPlayers, const std::string& gameMode) override {
            auto conn = dbPool_->get_connection();
            try {
                pqxx::work txn(*conn);

                pqxx::result result = txn.exec_params(
                    "INSERT INTO Rooms (room_name, creator_id, created_at, max_players, "
                    "game_mode, status) "
                    "VALUES ($1, $2, CURRENT_TIMESTAMP, $3, $4, 'open') RETURNING room_id",
                    roomName, creatorId, maxPlayers, gameMode);

                txn.commit();
                dbPool_->return_connection(conn);

                if (result.empty()) {
                    return -1;
                }

                return result[0][0].as<int>();
            }
            catch (const std::exception& e) {
                spdlog::error("Error creating room: {}", e.what());
                auto conn = dbPool_->get_connection();
                return -1;
            }
        }

        bool addPlayer(int roomId, int userId) override {
            auto conn = dbPool_->get_connection();
            try {
                pqxx::work txn(*conn);

                // Check if already joined
                pqxx::result checkResult = txn.exec_params(
                    "SELECT id FROM RoomPlayers "
                    "WHERE room_id = $1 AND user_id = $2 AND leave_time IS NULL",
                    roomId, userId);

                if (!checkResult.empty()) {
                    // Already joined
                    txn.commit();
                    dbPool_->return_connection(conn);
                    return true;
                }

                // Add new player
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

    // Factory method implementation
    std::unique_ptr<RoomRepository> RoomRepository::create(DbPool* dbPool) {
        return std::make_unique<RoomRepositoryImpl>(dbPool);
    }

} // namespace game_server