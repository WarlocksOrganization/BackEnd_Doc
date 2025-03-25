// controller/room_controller.cpp
// �� ��Ʈ�ѷ� ���� ����
// �� ����, ����, ��� ��ȸ ���� ��û�� ó���ϴ� ��Ʈ�ѷ�
#include "game_controller.h"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

namespace game_server {

    using json = nlohmann::json;

    GameController::GameController(std::shared_ptr<GameService> gameService)
        : gameService_(gameService) {
    }

    nlohmann::json GameController::handleRequest(json& request) {
        // ��û�� action �ʵ忡 ���� ������ �ڵ鷯 ȣ��
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
            return error_response.dump();
        }
    }

    nlohmann::json GameController::handleStartGame(json& request) {
        json response = gameService_->startGame(request);
        return response.dump();
    }

    nlohmann::json GameController::handleEndGame(json& request) {
        json response = gameService_->endGame(request);
        return response.dump();
    }
} // namespace game_server