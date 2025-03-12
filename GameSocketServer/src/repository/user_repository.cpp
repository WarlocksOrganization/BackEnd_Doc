// repository/user_repository.cpp
// 사용자 리포지토리 구현 파일
// 사용자 관련 데이터베이스 작업을 처리하는 리포지토리
#include "user_repository.h"
#include "../util/db_pool.h"
#include <pqxx/pqxx>
#include <spdlog/spdlog.h>

namespace game_server {

    // 리포지토리 구현체
    class UserRepositoryImpl : public UserRepository {
    public:
        explicit UserRepositoryImpl(DbPool* dbPool) : dbPool_(dbPool) {}

        std::optional<User> findById(int userId) override {
            auto conn = dbPool_->get_connection();
            try {
                pqxx::work txn(*conn);

                // ID로 사용자 정보 조회
                pqxx::result result = txn.exec_params(
                    "SELECT user_id, username, password, rating, total_games, total_wins, "
                    "       created_at, last_login "
                    "FROM Users WHERE user_id = $1",
                    userId);

                txn.commit();
                dbPool_->return_connection(conn);

                if (result.empty()) {
                    return std::nullopt;
                }

                // 조회 결과로 User 객체 생성
                User user;
                user.userId = result[0][0].as<int>();
                user.username = result[0][1].as<std::string>();
                user.passwordHash = result[0][2].as<std::string>();
                user.rating = result[0][3].as<int>();
                user.totalGames = result[0][4].as<int>();
                user.totalWins = result[0][5].as<int>();
                user.createdAt = result[0][6].as<std::string>();

                if (!result[0][7].is_null()) {
                    user.lastLogin = result[0][7].as<std::string>();
                }

                return user;
            }
            catch (const std::exception& e) {
                spdlog::error("Error finding user by ID: {}", e.what());
                dbPool_->return_connection(conn);
                return std::nullopt;
            }
        }

        std::optional<User> findByUsername(const std::string& username) override {
            auto conn = dbPool_->get_connection();
            try {
                pqxx::work txn(*conn);

                // 사용자명으로 사용자 정보 조회
                pqxx::result result = txn.exec_params(
                    "SELECT user_id, username, password, rating, total_games, total_wins, "
                    "       created_at, last_login "
                    "FROM Users WHERE username = $1",
                    username);

                txn.commit();
                dbPool_->return_connection(conn);

                if (result.empty()) {
                    return std::nullopt;
                }

                // 조회 결과로 User 객체 생성
                User user;
                user.userId = result[0][0].as<int>();
                user.username = result[0][1].as<std::string>();
                user.passwordHash = result[0][2].as<std::string>();
                user.rating = result[0][3].as<int>();
                user.totalGames = result[0][4].as<int>();
                user.totalWins = result[0][5].as<int>();
                user.createdAt = result[0][6].as<std::string>();

                if (!result[0][7].is_null()) {
                    user.lastLogin = result[0][7].as<std::string>();
                }
                dbPool_->return_connection(conn);
                return user;
            }
            catch (const std::exception& e) {
                spdlog::error("Error finding user by username: {}", e.what());
                dbPool_->return_connection(conn);
                return std::nullopt;
            }
        }

        int create(const std::string& username, const std::string& hashedPassword) override {
            auto conn = dbPool_->get_connection();
            try {
                pqxx::work txn(*conn);

                // 새 사용자 생성
                pqxx::result result = txn.exec_params(
                    "INSERT INTO Users (username, password, created_at) "
                    "VALUES ($1, $2, CURRENT_TIMESTAMP) RETURNING user_id",
                    username, hashedPassword);

                txn.commit();
                dbPool_->return_connection(conn);

                if (result.empty()) {
                    return -1;
                }

                return result[0][0].as<int>();
            }
            catch (const std::exception& e) {
                spdlog::error("Error creating user: {}", e.what());
                dbPool_->return_connection(conn);
                return -1;
            }
        }

        bool updateLastLogin(int userId) override {
            auto conn = dbPool_->get_connection();
            try {
                pqxx::work txn(*conn);

                // 마지막 로그인 시간 업데이트
                pqxx::result result = txn.exec_params(
                    "UPDATE Users SET last_login = CURRENT_TIMESTAMP "
                    "WHERE user_id = $1 RETURNING user_id",
                    userId);

                txn.commit();
                dbPool_->return_connection(conn);

                return !result.empty();
            }
            catch (const std::exception& e) {
                spdlog::error("Error updating last login: {}", e.what());
                dbPool_->return_connection(conn);
                return false;
            }
        }

        bool updateRating(int userId, int newRating) override {
            auto conn = dbPool_->get_connection();
            try {
                pqxx::work txn(*conn);

                // 사용자 레이팅 업데이트
                pqxx::result result = txn.exec_params(
                    "UPDATE Users SET rating = $1 WHERE user_id = $2 RETURNING user_id",
                    newRating, userId);

                txn.commit();
                dbPool_->return_connection(conn);

                return !result.empty();
            }
            catch (const std::exception& e) {
                spdlog::error("Error updating user rating: {}", e.what());
                dbPool_->return_connection(conn);
                return false;
            }
        }

        bool updateStats(int userId, bool isWin) override {
            auto conn = dbPool_->get_connection();
            try {
                pqxx::work txn(*conn);

                // 게임 통계 업데이트
                std::string query;
                if (isWin) {
                    query = "UPDATE Users SET total_games = total_games + 1, "
                        "total_wins = total_wins + 1 WHERE user_id = $1 RETURNING user_id";
                }
                else {
                    query = "UPDATE Users SET total_games = total_games + 1 "
                        "WHERE user_id = $1 RETURNING user_id";
                }

                pqxx::result result = txn.exec_params(query, userId);

                txn.commit();
                dbPool_->return_connection(conn);

                return !result.empty();
            }
            catch (const std::exception& e) {
                spdlog::error("Error updating user stats: {}", e.what());
                dbPool_->return_connection(conn);
                return false;
            }
        }

        std::vector<User> getTopPlayers(int limit) override {
            std::vector<User> users;
            auto conn = dbPool_->get_connection();

            try {
                pqxx::work txn(*conn);

                // 상위 플레이어 목록 조회 (레이팅 기준)
                pqxx::result result = txn.exec_params(
                    "SELECT user_id, username, password, rating, total_games, total_wins, "
                    "       created_at, last_login "
                    "FROM Users ORDER BY rating DESC LIMIT $1",
                    limit);

                txn.commit();
                dbPool_->return_connection(conn);

                // 조회 결과를 User 객체 리스트로 변환
                for (const auto& row : result) {
                    User user;
                    user.userId = row[0].as<int>();
                    user.username = row[1].as<std::string>();
                    user.passwordHash = row[2].as<std::string>();
                    user.rating = row[3].as<int>();
                    user.totalGames = row[4].as<int>();
                    user.totalWins = row[5].as<int>();
                    user.createdAt = row[6].as<std::string>();

                    if (!row[7].is_null()) {
                        user.lastLogin = row[7].as<std::string>();
                    }

                    users.push_back(user);
                }
            }
            catch (const std::exception& e) {
                spdlog::error("Error getting top players: {}", e.what());
                dbPool_->return_connection(conn);
            }

            return users;
        }

    private:
        DbPool* dbPool_;
    };

    // 팩토리 메서드 구현
    std::unique_ptr<UserRepository> UserRepository::create(DbPool* dbPool) {
        return std::make_unique<UserRepositoryImpl>(dbPool);
    }

} // namespace game_server