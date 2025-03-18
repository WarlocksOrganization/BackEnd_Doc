// repository/user_repository.cpp
// ����� �������丮 ���� ����
// ����� ���� �����ͺ��̽� �۾��� ó���ϴ� �������丮
#include "user_repository.h"
#include "../util/db_pool.h"
#include <pqxx/pqxx>
#include <spdlog/spdlog.h>

namespace game_server {

    using json = nlohmann::json;

    // �������丮 ����ü
    class UserRepositoryImpl : public UserRepository {
    public:
        explicit UserRepositoryImpl(DbPool* dbPool) : dbPool_(dbPool) {}

        //std::optional<json> findById(int userId) override {}

        json findByUsername(const std::string& username) {
            auto conn = dbPool_->get_connection();
            try {
                pqxx::work txn(*conn);
                pqxx::result result = txn.exec_params(
                    "SELECT * FROM users WHERE username = $1", username);

                if (result.empty()) {
                    // ����ڸ� ã�� ���� - std::nullopt ��ȯ
                    return { {"user_id", -1} };
                }

                // ����� JSON���� ��ȯ
                nlohmann::json user;
                user["user_id"] = result[0]["user_id"].as<int>();
                user["user_name"] = result[0]["user_name"].as<std::string>();
                user["password_hash"] = result[0]["password_hash"].as<std::string>();
                user["wins"] = result[0]["wins"].as<int>();
                user["games_played"] = result[0]["games_played"].as<int>();
                user["total_kills"] = result[0]["total_kills"].as<int>();
                user["total_damages"] = result[0]["total_damages"].as<int>();
                user["total_deaths"] = result[0]["total_deaths"].as<int>();
                user["rating"] = result[0]["rating"].as<int>();
                user["highest_rating"] = result[0]["highest_rating"].as<int>();
                user["create_at"] = result[0]["create_at"].as<std::string>();
                user["last_login"] = result[0]["last_login"].as<std::string>();

                txn.commit();
                dbPool_->return_connection(conn);
                return user;
            }
            catch (const std::exception& e) {
                dbPool_->return_connection(conn);
                spdlog::error("Database error in findByUsername: {}", e.what());
                return { {"user_id", -1} };
            }
        }

        int create(const std::string& username, const std::string& hashedPassword) override {
            auto conn = dbPool_->get_connection();
            try {
                pqxx::work txn(*conn);

                // �� ����� ����
                pqxx::result result = txn.exec_params(
                    "INSERT INTO users (user_name, password_hash) "
                    "VALUES ($1, $2) RETURNING user_id",
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

                // ������ �α��� �ð� ������Ʈ
                pqxx::result result = txn.exec_params(
                    "UPDATE users SET last_login = CURRENT_TIMESTAMP "
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

                // ����� ������ ������Ʈ
                pqxx::result result = txn.exec_params(
                    "UPDATE users SET rating = $1 WHERE user_id = $2 RETURNING user_id",
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

                // ���� ��� ������Ʈ
                std::string query;
                if (isWin) {
                    query = "UPDATE users SET games_played = games_played + 1, "
                        "wins = wins + 1 WHERE user_id = $1 RETURNING user_id";
                }
                else {
                    query = "UPDATE users SET games_played = games_played + 1 "
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

        std::vector<json> getTopPlayers(int limit) override {
            std::vector<json> users;
            auto conn = dbPool_->get_connection();

            try {
                pqxx::work txn(*conn);

                // ���� �÷��̾� ��� ��ȸ (������ ����)
                pqxx::result result = txn.exec_params(
                    "SELECT * "
                    "FROM users ORDER BY rating DESC LIMIT $1",
                    limit);

                txn.commit();
                dbPool_->return_connection(conn);

                // ��ȸ ����� User ��ü ����Ʈ�� ��ȯ
                for (const auto& row : result) {
                    nlohmann::json user;
                    user["user_id"] = row["user_id"].as<int>();
                    user["user_name"] = row["user_name"].as<std::string>();
                    user["password_hash"] = row["password_hash"].as<std::string>();
                    user["wins"] = row["wins"].as<int>();
                    user["games_played"] = row["games_played"].as<int>();
                    user["total_kills"] = row["total_kills"].as<int>();
                    user["total_damages"] = row["total_damages"].as<int>();
                    user["total_deaths"] = row["total_deaths"].as<int>();
                    user["rating"] = row["rating"].as<int>();
                    user["highest_rating"] = row["highest_rating"].as<int>();
                    user["create_at"] = row["create_at"].as<std::string>();
                    user["last_login"] = row["last_login"].as<std::string>();

                    users.push_back(user);
                }
                return users;
            }
            catch (const std::exception& e) {
                spdlog::error("Error getting top players: {}", e.what());
                dbPool_->return_connection(conn);
                return users;
            }
        }

    private:
        DbPool* dbPool_;
    };

    // ���丮 �޼��� ����
    std::unique_ptr<UserRepository> UserRepository::create(DbPool* dbPool) {
        return std::make_unique<UserRepositoryImpl>(dbPool);
    }

} // namespace game_server