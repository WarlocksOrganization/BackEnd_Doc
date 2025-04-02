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
                    spdlog::error("게임 세션을 생성할 수 없습니다");
                    txn.abort();
                    dbPool_->return_connection(conn);
                    return -1;
                }
                int gameId = result[0][0].as<int>();
                spdlog::info("게임 세션이 완전히 생성되었습니다! 게임 ID: {}", gameId);
                txn.commit();
                dbPool_->return_connection(conn);
                return gameId;
            }
            catch (const std::exception& e) {
                txn.abort();
                dbPool_->return_connection(conn);
                spdlog::error("createGame 데이터베이스 오류: {}", e.what());
                return -1;
            }
        }

        int endGame(int gameId) {
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
                    spdlog::error("게임 ID: {}에 해당하는 방 ID를 찾을 수 없습니다", gameId);
                    txn.abort();
                    dbPool_->return_connection(conn);
                    return -1;
                }
                int roomId = result[0][0].as<int>();
                spdlog::info("게임 ID: {}의 상태가 성공적으로 완료로 업데이트되었습니다", gameId);

                //pqxx::result res = txn.exec_params(
                //    "UPDATE rooms "
                //    "SET status = 'WAITING' "
                //    "WHERE room_id = $1 "
                //    "RETURNING room_id",
                //    roomId);

                //if (res.empty() || roomId != res[0][0].as<int>()) {
                //    spdlog::error("방 ID: {}의 상태 업데이트 실패", roomId);
                //    txn.abort();
                //    dbPool_->return_connection(conn);
                //    return -1;
                //}
                //spdlog::info("방 ID: {}의 상태가 완전히 업데이트되었습니다", roomId);

                txn.commit();
                dbPool_->return_connection(conn);
                return roomId;
            }
            catch (const std::exception& e) {
                txn.abort();
                dbPool_->return_connection(conn);
                spdlog::error("endGame 데이터베이스 오류: {}", e.what());
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