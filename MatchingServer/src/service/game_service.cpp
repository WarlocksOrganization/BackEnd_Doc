#include "game_service.h"
#include "../repository/game_repository.h"
#include <spdlog/spdlog.h>
#include <random>
#include <string>
#include <nlohmann/json.hpp>

namespace game_server {

    using json = nlohmann::json;

    // 서비스 구현체
    class GameServiceImpl : public GameService {
    public:
        explicit GameServiceImpl(std::shared_ptr<GameRepository> gameRepo)
            : gameRepo_(gameRepo) {
        }

        json startGame(json& request) {
            json response;
            try {
                // 요청 유효성 검증
                if (!request.contains("roomId") || !request.contains("mapId")) {
                    response["status"] = "error";
                    response["message"] = "Missing required fields in request";
                    return response;
                }

                // 게임 ID 얻기 실패 시 -1
                int gameId = gameRepo_->createGame(request);

                if (gameId == -1) {
                    response["status"] = "error";
                    response["message"] = "Failed to add new game recode";
                    return response;
                }

                // 성공 응답 생성
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
                // 요청 유효성 검증
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

                // 성공 응답 생성
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

    // 팩토리 메서드 구현
    std::unique_ptr<GameService> GameService::create(std::shared_ptr<GameRepository> gameRepo) {
        return std::make_unique<GameServiceImpl>(gameRepo);
    }

} // namespace game_server