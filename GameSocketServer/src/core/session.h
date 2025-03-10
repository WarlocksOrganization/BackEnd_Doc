// core/session.h
#pragma once
#include <boost/asio.hpp>
#include <memory>
#include <string>
#include <array>
#include <map>
#include "../controller/controller.h"

namespace game_server {

    class Session : public std::enable_shared_from_this<Session> {
    public:
        Session(boost::asio::ip::tcp::socket socket,
            std::map<std::string, std::shared_ptr<Controller>>& controllers);

        void start();

    private:
        void read_header();
        void read_body(std::size_t content_length);
        void process_request(const std::string& request_data);
        void write_response(const std::string& response);
        void handle_error(const std::string& error_message);

        boost::asio::ip::tcp::socket socket_;
        std::map<std::string, std::shared_ptr<Controller>>& controllers_;
        std::array<char, 8192> buffer_;
        std::string message_;
        int user_id_;
    };

} // namespace game_server