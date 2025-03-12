// core/server.h
#pragma once
#include <boost/asio.hpp>
#include <memory>
#include <string>
#include <map>
#include <unordered_map>
#include "../controller/controller.h"
#include "../util/db_pool.h"

namespace game_server {

    class Server {
    public:
        Server(boost::asio::io_context& io_context,
            short port,
            const std::string& db_connection_string);
        ~Server();

        void run();
        void stop();

    private:
        void do_accept();
        void init_controllers();

        boost::asio::io_context& io_context_;
        boost::asio::ip::tcp::acceptor acceptor_;
        std::unique_ptr<DbPool> db_pool_;
        std::map<std::string, std::shared_ptr<Controller>> controllers_;
        bool running_;
    };

} // namespace game_server