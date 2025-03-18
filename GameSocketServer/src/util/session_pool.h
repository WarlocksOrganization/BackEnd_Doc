//// core/session_pool.h
//#pragma once
//#include <boost/asio.hpp>
//#include <chrono>
//#include <map>
//#include <memory>
//#include <mutex>
//#include <thread>
//#include <string>
//#include <unordered_map>
//
//namespace game_server {
//
//    class Session;
//
//    class SessionPool : public std::enable_shared_from_this<SessionPool> {
//    public:
//        SessionPool(std::chrono::seconds session_timeout = std::chrono::minutes(30));
//        ~SessionPool();
//
//        // 세션 등록
//        void register_session(std::shared_ptr<Session> session);
//
//        // 세션 활동 업데이트
//        void update_activity(Session* session_ptr);
//
//        // 세션 제거
//        void remove_session(Session* session_ptr);
//
//        // 활성 세션 수 반환
//        size_t active_sessions_count() const;
//
//    private:
//        // 만료된 세션 정리 스레드 함수
//        void cleanup_expired_sessions();
//
//        // 세션 포인터로 세션 ID를 찾기 위한 맵
//        std::unordered_map<Session*, std::chrono::steady_clock::time_point> session_by_ptr_;
//
//        // 세션 타임아웃 시간
//        std::chrono::seconds session_timeout_;
//
//        // 스레드 동기화를 위한 뮤텍스
//        mutable std::mutex mutex_;
//
//        // 세션 정리 스레드
//        std::thread cleanup_thread_;
//
//        // 스레드 실행 상태
//        bool running_;
//    };
//
//} // namespace game_server