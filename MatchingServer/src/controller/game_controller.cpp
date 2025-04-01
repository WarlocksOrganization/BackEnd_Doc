// controller/room_controller.cpp
// 방 컨트롤러 구현 파일
// 방 생성, 참가, 목록 조회 등의 요청을 처리하는 컨트롤러
#include "game_controller.h"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

namespace game_server {

    using json = nlohmann::json;

    GameController::GameController(std::shared_ptr<GameService> gameService)
        : gameService_(gameService) {
    }

    nlohmann::json GameController::handleRequest(json& request) {
        // 요청의 action 필드에 따라 적절한 핸들러 호출
        std::string action = request["action"];

        if (action == "gameStart") {
            return handleStartGame(request);
        }
        else if (action == "gameEnd") {
            return handleEndGame(request);
        }
        else {
            json error_response = {
                {"status", "error"},
                {"message", "Unknown room action"}
            };
            return error_response;
        }
    }

    nlohmann::json GameController::handleStartGame(json& request) {
        json response = gameService_->startGame(request);
        return response;
    }

    nlohmann::json GameController::handleEndGame(json& request) {
        json response = gameService_->endGame(request);
        return response;
    }
} // namespace game_server
