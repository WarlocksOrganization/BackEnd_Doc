// core/server.cpp
#include "server.h"
#include "session.h"
#include "../controller/auth_controller.h"
#include "../controller/room_controller.h"
#include "../service/auth_service.h"
#include "../service/room_service.h"
#include "../repository/user_repository.h"
#include "../repository/room_repository.h"
#include <iostream>
#include <spdlog/spdlog.h>

namespace game_server {

    Server::Server(boost::asio::io_context& io_context,
        short port,
        const std::string& db_connection_string)
        : io_context_(io_context),
        acceptor_(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
        running_(false)
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

    void Server::init_controllers() {
        // Create repositories
        auto userRepo = UserRepository::create(db_pool_.get());
        auto roomRepo = RoomRepository::create(db_pool_.get());

        std::shared_ptr<UserRepository> sharedUserRepo = std::move(userRepo);
        std::shared_ptr<RoomRepository> sharedRoomRepo = std::move(roomRepo);

        // Create services
        auto authService = AuthService::create(sharedUserRepo);
        auto roomService = RoomService::create(sharedRoomRepo);

        // Create and register controllers
        controllers_["auth"] = std::make_shared<AuthController>(std::move(authService));
        controllers_["room"] = std::make_shared<RoomController>(std::move(roomService));

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
        spdlog::info("Server stopped");
    }

    void Server::do_accept()
    {
        acceptor_.async_accept(
            [this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
                if (!ec) {
                    // Create and start session
                    std::make_shared<Session>(std::move(socket), controllers_)->start();
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