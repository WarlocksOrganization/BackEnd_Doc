// core/server.h
#pragma once
#include <boost/asio.hpp>
#include <memory>
#include <string>
#include <map>
#include <unordered_map>
#include <mutex>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "../controller/controller.h"
#include "../util/db_pool.h"

namespace game_server {

    class Session;

    class Server {
    public:
        Server(boost::asio::io_context& io_context,
            short port,
            const std::string& db_connection_string,
            const std::string& VERSION);
        ~Server();

        void run();
        void stop();

        // 세션 관리 메서드
        std::string registerSession(std::shared_ptr<Session> session);
        void registerMirrorSession(std::shared_ptr<Session> session, int port);
        void removeSession(const std::string& token, int userId);
        void removeMirrorSession(int port);
        std::shared_ptr<Session> getSession(const std::string& token);
        std::shared_ptr<Session> getMirrorSession(int port);
        int getCCU();
        int getRoomCapacity();
        std::string generateSessionToken();
        void setSessionTimeout(std::chrono::seconds timeout);
        void startSessionTimeoutCheck();
        bool checkAlreadyLogin(int userId);
        std::string getServerVersion();
    private:
        void do_accept();
        void init_controllers();
        void check_inactive_sessions();

        boost::asio::io_context& io_context_;
        boost::asio::ip::tcp::acceptor acceptor_;
        std::unique_ptr<DbPool> db_pool_;
        std::map<std::string, std::shared_ptr<Controller>> controllers_;
        bool running_;

        // 세션 관리 데이터
        std::unordered_map<int, std::weak_ptr<Session>> mirrors_;
        std::mutex mirrors_mutex_;
        std::unordered_map<std::string, std::weak_ptr<Session>> sessions_;
        std::mutex sessions_mutex_;
        std::unordered_map<int, std::string> tokens_;
        std::mutex tokens_mutex_;
        boost::uuids::random_generator uuid_generator_;
        std::chrono::seconds session_timeout_{ 12 }; // 기본 12초
        boost::asio::steady_timer session_check_timer_;
        bool timeout_check_running_{ false };
        
        // 버전 관리 데이터
        std::string version_;
    };

} // namespace game_server
