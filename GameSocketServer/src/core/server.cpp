// core/server.cpp
#include "server.h"
#include "session.h"
#include "../controller/auth_controller.h"
#include "../service/auth_service.h"
#include "../repository/user_repository.h"
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
        // 데이터베이스 연결 풀 초기화
        db_pool_ = std::make_unique<DbPool>(db_connection_string, 5); // 5개의 연결 생성

        // 컨트롤러 초기화
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
        // 리포지토리 생성
        auto userRepo = UserRepository::create(db_pool_.get());
        std::shared_ptr<UserRepository> sharedUserRepo = std::move(userRepo);

        // 서비스 생성
        auto authService = AuthService::create(sharedUserRepo);

        // 컨트롤러 생성 및 등록
        controllers_["auth"] = std::make_shared<AuthController>(std::move(authService));

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
                    // 새 세션 생성 및 시작
                    std::make_shared<Session>(std::move(socket), controllers_)->start();
                }
                else {
                    spdlog::error("Error accepting connection: {}", ec.message());
                }

                // 계속해서 연결 수락 (서버가 실행 중인 경우)
                if (running_) {
                    do_accept();
                }
            }
        );
    }

} // namespace game_server