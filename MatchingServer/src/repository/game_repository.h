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

        virtual int createGame(int room_id, int map_id) = 0;
        virtual int endGame(int room_id) = 0;
        virtual bool insertGameUsers(int gameId, const json& users) = 0;

        static std::unique_ptr<GameRepository> create(DbPool* dbPool);
    };

} // namespace game_server