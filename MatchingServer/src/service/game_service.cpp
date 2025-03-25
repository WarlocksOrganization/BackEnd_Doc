#include "game_service.h"
#include "../repository/game_repository.h"
#include <spdlog/spdlog.h>
#include <random>
#include <string>
#include <nlohmann/json.hpp>

namespace game_server {

    using json = nlohmann::json;

    // ���� ����ü
    class GameServiceImpl : public GameService {
    public:
        explicit GameServiceImpl(std::shared_ptr<GameRepository> gameRepo)
            : gameRepo_(gameRepo) {
        }

        json startGame(json& request) {
            json response;
            try {
                // ��û ��ȿ�� ����
                if (!request.contains("roomId") || !request.contains("mapId")) {
                    response["status"] = "error";
                    response["message"] = "Missing required fields in request";
                    return response;
                }

                // ���� ID ��� ���� �� -1
                int gameId = gameRepo_->createGame(request);

                if (gameId == -1) {
                    response["status"] = "error";
                    response["message"] = "Failed to add new game recode";
                    return response;
                }

                // ���� ���� ����
                response["action"] = "gameStart";
                response["status"] = "success";
                response["message"] = "game successfully created";
                response["gameId"] = gameId;

                spdlog::info("Room {} created new gameId: {}",
                    request["roomId"].get<int>(), gameId);

                return response;
            }
            catch (const std::exception& e) {
                response["status"] = "error";
                response["message"] = std::string("Error creating game : ") + e.what();
                spdlog::error("Error in createGame: {}", e.what());
                return response;
            }
        }

        json endGame(json& request) {
            json response;
            try {
                // ��û ��ȿ�� ����
                if (!request.contains("gameId")) {
                    response["status"] = "error";
                    response["message"] = "Missing required fields in request";
                    return response;
                }

                if (!gameRepo_->endGame(request["gameId"])) {
                    response["status"] = "error";
                    response["message"] = "Failed to game end update";
                    return response;
                }

                // ���� ���� ����
                response["action"] = "gameEnd";
                response["status"] = "success";
                response["message"] = "The game is ended successfully";

                spdlog::info("Room {} ended the gameId: {}",
                    request["roomId"].get<int>(), request["gameId"].get<int>());
                return response;
            }
            catch (const std::exception& e) {
                response["status"] = "error";
                response["message"] = std::string("Error end game : ") + e.what();
                spdlog::error("Error in endGame: {}", e.what());
                return response;
            }
        }

    private:
        std::shared_ptr<GameRepository> gameRepo_;
    };

    // ���丮 �޼��� ����
    std::unique_ptr<GameService> GameService::create(std::shared_ptr<GameRepository> gameRepo) {
        return std::make_unique<GameServiceImpl>(gameRepo);
    }

} // namespace game_server