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

        json findByUsername(const std::string& userName) {
            auto conn = dbPool_->get_connection();
            pqxx::work txn(*conn);
            try {
                pqxx::result result = txn.exec_params(
                    "SELECT user_id, user_name, password_hash, created_at, last_login FROM users WHERE LOWER(user_name) = LOWER($1)",
                    userName);

                if (result.empty()) {
                    // ����ڸ� ã�� ���� - std::nullopt ��ȯ
                    txn.abort();
                    dbPool_->return_connection(conn);
                    return { {"userId", -1} };
                }

                // ����� JSON���� ��ȯ
                nlohmann::json user;
                user["userId"] = result[0]["user_id"].as<int>();
                user["userName"] = result[0]["user_name"].as<std::string>();
                user["passwordHash"] = result[0]["password_hash"].as<std::string>();
                user["createdAt"] = result[0]["created_at"].as<std::string>();
                user["lastLogin"] = result[0]["last_login"].as<std::string>();

                txn.commit();
                dbPool_->return_connection(conn);
                return user;
            }
            catch (const std::exception& e) {
                txn.abort();
                dbPool_->return_connection(conn);
                spdlog::error("Database error in findByUsername: {}", e.what());
                return { {"userId", -1} };
            }
        }

        int create(const std::string& userName, const std::string& hashedPassword) override {
            auto conn = dbPool_->get_connection();
            pqxx::work txn(*conn);
            try {
                // �� ����� ����
                pqxx::result result = txn.exec_params(
                    "INSERT INTO users (user_name, password_hash) "
                    "VALUES ($1, $2) RETURNING user_id",
                    userName, hashedPassword);

                txn.commit();
                dbPool_->return_connection(conn);

                if (result.empty()) {
                    txn.abort();
                    dbPool_->return_connection(conn);
                    return -1;
                }

                return result[0][0].as<int>();
            }
            catch (const std::exception& e) {
                txn.abort();
                spdlog::error("Error creating user: {}", e.what());
                dbPool_->return_connection(conn);
                return -1;
            }
        }

        bool updateLastLogin(int userId) override {
            auto conn = dbPool_->get_connection();
            pqxx::work txn(*conn);
            try {
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
                txn.abort();
                spdlog::error("Error updating last login: {}", e.what());
                dbPool_->return_connection(conn);
                return false;
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