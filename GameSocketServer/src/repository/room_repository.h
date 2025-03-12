// repository/room_repository.h
#pragma once
#include "../entity/room.h"
#include "../entity/room_player.h"
#include <string>
#include <vector>
#include <optional>
#include <memory>

namespace game_server {

    class DbPool;

    class RoomRepository {
    public:
        virtual ~RoomRepository() = default;

        virtual std::optional<Room> findById(int roomId) = 0;
        virtual std::optional<Room> findByName(const std::string& roomName) = 0;
        virtual std::vector<Room> findAllOpen(int limit = 20) = 0;
        virtual int create(const std::string& roomName, int creatorId, int maxPlayers) = 0;
        virtual bool addPlayer(int roomId, int userId) = 0;
        virtual bool removePlayer(int roomId, int userId) = 0;
        virtual int getPlayerCount(int roomId) = 0;
        virtual std::vector<int> getPlayersInRoom(int roomId) = 0;

        static std::unique_ptr<RoomRepository> create(DbPool* dbPool);
    };

} // namespace game_server