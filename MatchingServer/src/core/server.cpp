// core/server.cpp
#include "server.h"
#include "session.h"
#include "../controller/auth_controller.h"
#include "../controller/room_controller.h"
#include "../controller/game_controller.h"
#include "../service/auth_service.h"
#include "../service/room_service.h"
#include "../service/game_service.h"
#include "../repository/user_repository.h"
#include "../repository/room_repository.h"
#include "../repository/game_repository.h"
#include <iostream>
#include <spdlog/spdlog.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace game_server {

    Server::Server(boost::asio::io_context& io_context,
        short port,
        const std::string& db_connection_string)
        : io_context_(io_context),
        acceptor_(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
        running_(false),
        uuid_generator_()
    {
        // Create database connection pool
        db_pool_ = std::make_unique<DbPool>(db_connection_string, 5); // Create 5 connections

        // Initialize controllers
        init_controllers();

        spdlog::info("Server initialized on port {}", port);
    }
    
    Server::~Server()
    {
        if (running_) {
            stop();
        }
    }

    std::string Server::generateSessionToken() {
        boost::uuids::uuid uuid = uuid_generator_();
        return boost::uuids::to_string(uuid);
    }

    std::string Server::registerSession(std::shared_ptr<Session> session) {
        std::lock_guard<std::mutex> lock(sessions_mutex_);

        // 기존 세션이 존재하면 제거
        for (auto it = sessions_.begin(); it != sessions_.end(); ++it) {
            if (it->second == session) {
                spdlog::info("Existing session found, removing old token: {}", it->first);
                sessions_.erase(it);
                break;  // 한 개만 삭제하면 되므로 루프 종료
            }
        }

        std::string token = generateSessionToken();
        sessions_[token] = session;
        spdlog::info("Session registered with token: {}", token);
        return token;
    }

    void Server::removeSession(const std::string& token) {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        auto it = sessions_.find(token);
        if (it != sessions_.end()) {
            sessions_.erase(it);
            spdlog::info("Session removed: {}", token);
        }
    }

    std::shared_ptr<Session> Server::getSession(const std::string& token) {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        auto it = sessions_.find(token);
        if (it != sessions_.end()) {
            return it->second;
        }
        return nullptr;
    }

    void Server::init_controllers() {
        // Create repositories
        auto userRepo = UserRepository::create(db_pool_.get());
        auto roomRepo = RoomRepository::create(db_pool_.get());
        auto gameRepo = GameRepository::create(db_pool_.get());

        std::shared_ptr<UserRepository> sharedUserRepo = std::move(userRepo);
        std::shared_ptr<RoomRepository> sharedRoomRepo = std::move(roomRepo);
        std::shared_ptr<GameRepository> sharedGameRepo = std::move(gameRepo);

        // Create services
        auto authService = AuthService::create(sharedUserRepo);
        auto roomService = RoomService::create(sharedRoomRepo);
        auto gameService = GameService::create(sharedGameRepo);

        // Create and register controllers
        controllers_["auth"] = std::make_shared<AuthController>(std::move(authService));
        controllers_["room"] = std::make_shared<RoomController>(std::move(roomService));
        controllers_["game"] = std::make_shared<GameController>(std::move(gameService));

        spdlog::info("Controllers initialized");
    }

    void Server::run()
    {
        running_ = true;
        do_accept();
        spdlog::info("Server is running and accepting connections...");
    }

    void Server::stop()
    {
        running_ = false;
        acceptor_.close();
        // 모든 세션 정리
        {
            std::lock_guard<std::mutex> lock(sessions_mutex_);
            sessions_.clear();
        }

        spdlog::info("Server stopped");
    }

    void Server::do_accept()
    {
        acceptor_.async_accept(
            [this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
                if (!ec) {
                    // Create and start session
                    auto session = std::make_shared<Session>(std::move(socket), controllers_, this);
                    session->start();
                }
                else {
                    spdlog::error("Connection acceptance error: {}", ec.message());
                }

                // Continue accepting connections (if server is still running)
                if (running_) {
                    do_accept();
                }
            }
        );
    }

} // namespace game_server