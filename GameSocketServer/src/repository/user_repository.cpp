// repository/user_repository.cpp
#include "user_repository.h"
#include "../util/db_pool.h"
#include <pqxx/pqxx>
#include <spdlog/spdlog.h>

namespace game_server {

    // 리포지토리 구현
    class UserRepositoryImpl : public UserRepository {
    public:
        explicit UserRepositoryImpl(DbPool* dbPool) : dbPool_(dbPool) {}

        std::optional<User> findById(int userId) override {
            try {
                auto conn = dbPool_->get_connection();
                pqxx::work txn(*conn);

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
                return std::nullopt;
            }
        }

        std::optional<User> findByUsername(const std::string& username) override {
            try {
                auto conn = dbPool_->get_connection();
                pqxx::work txn(*conn);

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
                spdlog::error("Error finding user by username: {}", e.what());
                return std::nullopt;
            }
        }

        int create(const std::string& username, const std::string& hashedPassword) override {
            try {
                auto conn = dbPool_->get_connection();
                pqxx::work txn(*conn);

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
                return -1;
            }
        }

        bool updateLastLogin(int userId) override {
            try {
                auto conn = dbPool_->get_connection();
                pqxx::work txn(*conn);

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
                return false;
            }
        }

        bool updateRating(int userId, int newRating) override {
            try {
                auto conn = dbPool_->get_connection();
                pqxx::work txn(*conn);

                pqxx::result result = txn.exec_params(
                    "UPDATE Users SET rating = $1 WHERE user_id = $2 RETURNING user_id",
                    newRating, userId);

                txn.commit();
                dbPool_->return_connection(conn);

                return !result.empty();
            }
            catch (const std::exception& e) {
                spdlog::error("Error updating user rating: {}", e.what());
                return false;
            }
        }

        bool updateStats(int userId, bool isWin) override {
            try {
                auto conn = dbPool_->get_connection();
                pqxx::work txn(*conn);

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
                return false;
            }
        }

        std::vector<User> getTopPlayers(int limit) override {
            std::vector<User> users;

            try {
                auto conn = dbPool_->get_connection();
                pqxx::work txn(*conn);

                pqxx::result result = txn.exec_params(
                    "SELECT user_id, username, password, rating, total_games, total_wins, "
                    "       created_at, last_login "
                    "FROM Users ORDER BY rating DESC LIMIT $1",
                    limit);

                txn.commit();
                dbPool_->return_connection(conn);

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