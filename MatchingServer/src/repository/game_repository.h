#pragma once
#include <optional>
#include <string>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>

namespace game_server {

    class DbPool;

    class GameRepository {
    public:
        virtual ~GameRepository() = default;

        virtual int createGame(const nlohmann::json& request) = 0;
        virtual int endGame(int roomId) = 0;

        static std::unique_ptr<GameRepository> create(DbPool* dbPool);
    };

} // namespace game_server