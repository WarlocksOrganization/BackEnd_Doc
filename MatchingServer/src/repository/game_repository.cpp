#include "game_repository.h"
#include "../util/db_pool.h"
#include <pqxx/pqxx>
#include <spdlog/spdlog.h>

namespace game_server {

    using json = nlohmann::json;

    // 리포지토리 구현체
    class GameRepositoryImpl : public GameRepository {
    public:
        explicit GameRepositoryImpl(DbPool* dbPool) : dbPool_(dbPool) {}

        int createGame(const json& request) {
            auto conn = dbPool_->get_connection();
            pqxx::work txn(*conn);
            try {
                int roomId = request["roomId"];
                int mapId = request["mapId"];

                pqxx::result result = txn.exec_params(
                    "INSERT INTO games (room_id, map_id) "
                    "VALUES ($1, $2) "
                    "RETURNING game_id",
                roomId, mapId);

                pqxx::result updateRoom = txn.exec_params(
                    "UPDATE rooms "
                    "SET status = 'GAME_IN_PROGRESS' "
                    "WHERE room_id = $1 "
                    "RETURNING status",
                    roomId);

                if (result.empty() || updateRoom.empty()) {
                    spdlog::error("Can't created game session");
                    txn.abort();
                    dbPool_->return_connection(conn);
                    return -1;
                }
                int gameId = result[0][0].as<int>();
                spdlog::info("Created game session completely! Game ID : {}", gameId);
                txn.commit();
                dbPool_->return_connection(conn);
                return gameId;
            }
            catch (const std::exception& e) {
                txn.abort();
                dbPool_->return_connection(conn);
                spdlog::error("Database error in createGame : {}", e.what());
                return -1;
            }
        }

        int endGame(int gameId)  {
            auto conn = dbPool_->get_connection();
            pqxx::work txn(*conn);
            try {
                pqxx::result result = txn.exec_params(
                    "UPDATE games "
                    "SET status = 'COMPLETED', completed_at = CURRENT_TIMESTAMP "
                    "WHERE game_id = $1 "
                    "RETURNING room_id",
                    gameId);

                if (result.empty()) {
                    spdlog::error("Can't found roomId use to Game ID : {}", gameId);
                    txn.abort();
                    dbPool_->return_connection(conn);
                    return -1;
                }
                int roomId = result[0][0].as<int>();
                spdlog::info("found roomId completely! Room ID : {}", roomId);

                pqxx::result res = txn.exec_params(
                    "UPDATE rooms "
                    "SET status = 'WAITING' "
                    "WHERE room_id = $1 "
                    "RETURNING room_id",
                    roomId);

                if (res.empty()) {
                    spdlog::error("Can't found roomId use to Room ID : {}", roomId);
                    txn.abort();
                    dbPool_->return_connection(conn);
                    return -1;
                }
                int resId = res[0][0].as<int>();
                if (roomId != resId) {
                    spdlog::error("Can't found room which Room ID is {}", roomId);
                    txn.abort();
                    dbPool_->return_connection(conn);
                    return -1;
                }

                spdlog::info("Change room status competely Room ID : {}", roomId);

                txn.commit();
                dbPool_->return_connection(conn);
                return roomId;
            }
            catch (const std::exception& e) {
                txn.abort();
                dbPool_->return_connection(conn);
                spdlog::error("Database error in findByUsername: {}", e.what());
                return -1;
            }
        }
        

    private:
        DbPool* dbPool_;
    };

    // 팩토리 메서드 구현
    std::unique_ptr<GameRepository> GameRepository::create(DbPool* dbPool) {
        return std::make_unique<GameRepositoryImpl>(dbPool);
    }

} // namespace game_server