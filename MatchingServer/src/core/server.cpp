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
        uuid_generator_(),
        session_check_timer_(io_context)
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

    void Server::setSessionTimeout(std::chrono::seconds timeout) {
        session_timeout_ = timeout;
        spdlog::info("Session timeout set to {} seconds", timeout.count());
    }

    void Server::startSessionTimeoutCheck() {
        if (timeout_check_running_) return;
        timeout_check_running_ = true;
        check_inactive_sessions();
    }

    void Server::check_inactive_sessions() {
        if (!running_ || !timeout_check_running_) return;
        spdlog::debug("Checking for inactive sessions...");

        std::vector<std::string> sessionsToRemove;
        {
            std::lock_guard<std::mutex> lock(sessions_mutex_);

            for (const auto& [token, session] : sessions_) {
                if (!session->isActive(session_timeout_)) {
                    spdlog::info("Session {} timed out after {} seconds of inactivity",
                        token, session_timeout_.count());
                    sessionsToRemove.push_back(token);
                }
            }
        }

        for (const auto& token : sessionsToRemove) {
            std::shared_ptr<Session> session;
            {
                std::lock_guard<std::mutex> lock(sessions_mutex_);
                auto it = sessions_.find(token);
                if (it != sessions_.end()) {
                    session = it->second;
                    sessions_.erase(it);  // 컬렉션에서 세션 제거
                    spdlog::info("Session {} removed from server", token);
                }
            }

            if (session) {
                session->handle_error("Session timed out");
            }
        }

        session_check_timer_.expires_after(std::chrono::seconds(60));
        session_check_timer_.async_wait([this](const boost::system::error_code& ec) {
            if (!ec) {
                check_inactive_sessions();
            }
            });
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
        startSessionTimeoutCheck();
        spdlog::info("Server is running and accepting connections...");
    }

    void Server::stop() {
        if (!running_) return;  // 이미 중지된 경우 중복 실행 방지

        running_ = false;
        timeout_check_running_ = false;

        // 타이머 취소 및 대기
        session_check_timer_.cancel();

        // 모든 세션에 종료 알림
        {
            std::lock_guard<std::mutex> lock(sessions_mutex_);
            for (auto& [token, session] : sessions_) {
                try {
                    if (session) {
                        session->handle_error("Server shutting down");
                    }
                }
                catch (const std::exception& e) {
                    spdlog::error("Error during session cleanup: {}", e.what());
                }
            }
            sessions_.clear();
        }

        // acceptor 닫기
        try {
            if (acceptor_.is_open()) {
                acceptor_.close();
            }
        }
        catch (const std::exception& e) {
            spdlog::error("Error closing acceptor: {}", e.what());
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
                    session->initialize();
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