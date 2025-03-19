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
//        // ���� ���
//        void register_session(std::shared_ptr<Session> session);
//
//        // ���� Ȱ�� ������Ʈ
//        void update_activity(Session* session_ptr);
//
//        // ���� ����
//        void remove_session(Session* session_ptr);
//
//        // Ȱ�� ���� �� ��ȯ
//        size_t active_sessions_count() const;
//
//    private:
//        // ����� ���� ���� ������ �Լ�
//        void cleanup_expired_sessions();
//
//        // ���� �����ͷ� ���� ID�� ã�� ���� ��
//        std::unordered_map<Session*, std::chrono::steady_clock::time_point> session_by_ptr_;
//
//        // ���� Ÿ�Ӿƿ� �ð�
//        std::chrono::seconds session_timeout_;
//
//        // ������ ����ȭ�� ���� ���ؽ�
//        mutable std::mutex mutex_;
//
//        // ���� ���� ������
//        std::thread cleanup_thread_;
//
//        // ������ ���� ����
//        bool running_;
//    };
//
//} // namespace game_server