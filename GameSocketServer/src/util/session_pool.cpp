// core/session_pool.cpp
#include "session_pool.h"
#include "session.h"
#include <spdlog/spdlog.h>

namespace game_server {

    SessionPool::SessionPool(std::chrono::seconds session_timeout)
        : session_timeout_(session_timeout),
        running_(true) {

        // 세션 클린업 스레드 시작
        cleanup_thread_ = std::thread(&SessionPool::cleanup_expired_sessions, this);
        spdlog::info("Session pool initialized with {} minute timeout",
            std::chrono::duration_cast<std::chrono::minutes>(session_timeout).count());
    }

    SessionPool::~SessionPool() {
        running_ = false;
        if (cleanup_thread_.joinable()) {
            cleanup_thread_.join();
        }
        spdlog::info("Session pool destroyed");
    }

    void SessionPool::register_session(std::shared_ptr<Session> session) {
        std::lock_guard<std::mutex> lock(mutex_);
        session_by_ptr_[session.get()] = std::chrono::steady_clock::now();
        spdlog::debug("Session registered");
    }

    void SessionPool::update_activity(Session* session_ptr) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = session_by_ptr_.find(session_ptr);
        if (it != session_by_ptr_.end()) {
            session_by_ptr_[session_ptr] = std::chrono::steady_clock::now();
            spdlog::debug("Session activity updated");
        }
    }

    void SessionPool::remove_session(Session* session_ptr) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = session_by_ptr_.find(session_ptr);
        if (it != session_by_ptr_.end()) {
            session_by_ptr_.erase(it);
            spdlog::debug("Session removed");
        }
    }

    size_t SessionPool::active_sessions_count() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return session_by_ptr_.size();
    }

    void SessionPool::cleanup_expired_sessions() {
        while (running_) {
            // 10초마다 만료된 세션 체크
            std::this_thread::sleep_for(std::chrono::seconds(10));

            std::vector<Session*> expired_sessions;

            {
                std::lock_guard<std::mutex> lock(mutex_);
                auto now = std::chrono::steady_clock::now();

                // 만료된 세션 식별
                for (auto it = session_by_ptr_.begin(); it != session_by_ptr_.end();) {
                    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - it->second);

                    if (elapsed > session_timeout_) {
                        spdlog::info("Session expired");
                        // 세션 포인터 저장
                        expired_sessions.push_back(it->first);

                        // 세션 맵에서 제거
                        it = session_by_ptr_.erase(it);
                    }
                    else {
                        ++it;
                    }
                }
            }

            // 잠금 밖에서 만료된 세션 정리
            for (auto* session : expired_sessions) {
                try {
                    // Session 클래스에 close_connection 메서드가 있다고 가정
                    boost::asio::ip::tcp::socket& socket = session->get_socket();
                    if (socket.is_open()) {
                        boost::system::error_code ec;
                        socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
                        socket.close(ec);

                        if (ec) {
                            spdlog::error("Socket closing error: {}", ec.message());
                        }
                    }
                }
                catch (const std::exception& e) {
                    spdlog::error("Error closing expired session: {}", e.what());
                }
            }

            if (!expired_sessions.empty()) {
                spdlog::info("Removed {} expired sessions, remaining: {}",
                    expired_sessions.size(), session_by_ptr_.size());
            }
        }
    }

} // namespace game_server