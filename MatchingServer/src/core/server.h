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
            const std::string& db_connection_string);
        ~Server();

        void run();
        void stop();

        // 技记 包府 皋辑靛
        std::string registerSession(std::shared_ptr<Session> session);
        void removeSession(const std::string& token, int userId);
        std::shared_ptr<Session> getSession(const std::string& token);
        std::string generateSessionToken();
        void setSessionTimeout(std::chrono::seconds timeout);
        void startSessionTimeoutCheck();
        std::unordered_map<int, std::shared_ptr<Session>> mirrors_;
        bool checkAlreadyLogin(int userId);
    private:
        void do_accept();
        void init_controllers();
        void check_inactive_sessions();

        boost::asio::io_context& io_context_;
        boost::asio::ip::tcp::acceptor acceptor_;
        std::unique_ptr<DbPool> db_pool_;
        std::map<std::string, std::shared_ptr<Controller>> controllers_;
        bool running_;

        // 技记 包府 单捞磐
        std::unordered_map<std::string, std::shared_ptr<Session>> sessions_;
        std::mutex sessions_mutex_;
        std::unordered_map<int, std::string> tokens_;
        std::mutex tokens_mutex_;
        boost::uuids::random_generator uuid_generator_;
        std::chrono::seconds session_timeout_{ 12 }; // 扁夯 12檬
        boost::asio::steady_timer session_check_timer_;
        bool timeout_check_running_{ false };
    };

} // namespace game_server