﻿// core/server.cpp
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
        // DB풀 생성
        db_pool_ = std::make_unique<DbPool>(db_connection_string, 20);

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

    bool Server::checkAlreadyLogin(int userId) {
        std::lock_guard<std::mutex> lock(tokens_mutex_);
        return tokens_.count(userId) > 0;
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
            for (const auto& [token, wsession] : sessions_) {
                auto session = wsession.lock();
                if (!session) {
                    // 세션이 이미 소멸됨
                    spdlog::info("Session {} already expired", token);
                    sessionsToRemove.push_back(token);
                }
                else if (!session->isActive(session_timeout_)) {
                    // 세션이 존재하지만 타임아웃됨
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
                    session = it->second.lock();
                    sessions_.erase(it);  // 컬렉션에서 세션 제거
                    spdlog::info("Session {} removed from server", token);
                }
            }

            if (session) {
                session->handle_error("Session timed out");
            }
        }

        session_check_timer_.expires_after(std::chrono::seconds(10));
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
            if (it->second.lock() == session) {
                spdlog::info("Existing session found, removing old token: {}", it->first);
                sessions_.erase(it);
                break;  // 한 개만 삭제하면 되므로 루프 종료
            }
        }

        std::string token = generateSessionToken();
        sessions_[token] = session;
        int userId = session->getUserId();
        if (userId) {
            tokens_[userId] = token;
            spdlog::info("Session user ID {} registered with token: {}", userId, token);
        }
        return token;
    }

    void Server::registerMirrorSession(std::shared_ptr<Session> session, int port) {
        std::lock_guard<std::mutex> lock(mirrors_mutex_);

        // 기존 세션이 존재하면 제거
        for (auto it = mirrors_.begin(); it != mirrors_.end(); ++it) {
            if (it->second.lock() == session) {
                spdlog::info("Existing mirror session found, removing old session: {}", it->first);
                mirrors_.erase(it);
                break;  // 한 개만 삭제하면 되므로 루프 종료
            }
        }

        mirrors_[port] = session;
        return;
    }

    void Server::removeSession(const std::string& token, int userId) {
        std::lock_guard<std::mutex> session_lock(sessions_mutex_);
        std::lock_guard<std::mutex> token_lock(tokens_mutex_);

        bool found = false;
        auto it_session = sessions_.find(token);
        if (it_session != sessions_.end()) {
            sessions_.erase(it_session);
            found = true;
        }

        auto it_token = tokens_.find(userId);
        if (it_token != tokens_.end()) {
            tokens_.erase(it_token);
            found = true;
        }
        if (found) {
            spdlog::info("Session removed token={}, userId={}", token, userId);
        }
    }

    void Server::removeMirrorSession(int port) {
        std::lock_guard<std::mutex> lock(mirrors_mutex_);
        auto it = mirrors_.find(port);
        if (it != mirrors_.end()) {
            mirrors_.erase(it);
            spdlog::info("Mirror session removed port: {}", port);
        }
    }

    std::shared_ptr<Session> Server::getSession(const std::string& token) {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        auto it = sessions_.find(token);
        if (it != sessions_.end()) {
            auto session = it->second.lock();
            if (session) {
                return session;
            }
            else {
                // 세션이 이미 소멸된 경우 맵에서 제거
                sessions_.erase(it);
                spdlog::info("Removed expired session from map: {}", token);
            }
        }
        return nullptr;
    }

    std::shared_ptr<Session> Server::getMirrorSession(int port) {
        std::lock_guard<std::mutex> lock(mirrors_mutex_);
        auto it = mirrors_.find(port);
        if (it != mirrors_.end()) {
            auto session = it->second.lock();
            if (session) {
                return session;
            }
            else {
                // 세션이 이미 소멸된 경우 맵에서 제거
                mirrors_.erase(it);
                spdlog::info("Removed expired mirror session from map port: {}", port);
            }
        }
        return nullptr;
    }

    int Server::getCCU() {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        return sessions_.size();
    }

    int Server::getRoomCapacity() {
        std::lock_guard<std::mutex> lock(mirrors_mutex_);
        return mirrors_.size();
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
            for (auto& [token, wsession] : sessions_) {
                auto session = wsession.lock();
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
