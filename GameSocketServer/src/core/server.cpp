// core/server.cpp
// 서버 클래스 구현 파일
// 서버 초기화, 실행, 종료 및 컨트롤러 초기화를 담당
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
        // 데이터베이스 연결 풀 생성
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
        auto roomRepo = RoomRepository::create(db_pool_.get());

        std::shared_ptr<UserRepository> sharedUserRepo = std::move(userRepo);
        std::shared_ptr<RoomRepository> sharedRoomRepo = std::move(roomRepo);

        // 서비스 생성
        auto authService = AuthService::create(sharedUserRepo);
        auto roomService = RoomService::create(sharedRoomRepo);

        // 컨트롤러 생성 및 등록
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
        // 비동기적으로 클라이언트 연결 수락
        acceptor_.async_accept(
            [this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
                if (!ec) {
                    // 세션 생성 및 시작
                    std::make_shared<Session>(std::move(socket), controllers_)->start();
                }
                else {
                    spdlog::error("Connection acceptance error: {}", ec.message());
                }

                // 서버가 실행 중인 경우 계속해서 연결 수락
                if (running_) {
                    do_accept();
                }
            }
        );
    }

} // namespace game_server