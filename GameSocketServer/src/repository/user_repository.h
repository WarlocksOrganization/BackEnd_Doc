// repository/user_repository.h
#pragma once
#include <optional>
#include <string>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>

namespace game_server {

    class DbPool;

    class UserRepository {
    public:
        virtual ~UserRepository() = default;

        //virtual std::optional<nlohmann::json> findById(int userId) = 0;
        virtual nlohmann::json findByUsername(const std::string& username) = 0;
        virtual int create(const std::string& username, const std::string& hashedPassword) = 0;
        virtual bool updateLastLogin(int userId) = 0;
        virtual bool updateRating(int userId, int newRating) = 0;
        virtual bool updateStats(int userId, bool isWin) = 0;
        virtual std::vector<nlohmann::json> getTopPlayers(int limit) = 0;

        static std::unique_ptr<UserRepository> create(DbPool* dbPool);
    };

} // namespace game_server