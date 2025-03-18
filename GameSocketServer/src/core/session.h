// core/session.h
#pragma once
#include "../controller/controller.h"
#include <boost/asio.hpp>
#include <memory>
#include <string>
#include <array>
#include <map>
#include <nlohmann/json.hpp>

namespace game_server {

    using json = nlohmann::json;

    class Session : public std::enable_shared_from_this<Session> {
    public:
        Session(boost::asio::ip::tcp::socket socket,
            std::map<std::string, std::shared_ptr<Controller>>& controllers);

        void start();
        boost::asio::ip::tcp::socket& get_socket();

    private:
        void read_message();
        void process_request(json& request);
        void write_response(const std::string& response);
        void handle_error(const std::string& error_message);

        boost::asio::ip::tcp::socket socket_;
        std::map<std::string, std::shared_ptr<Controller>>& controllers_;
        std::array<char, 8192> buffer_;
        std::string message_;
        int user_id_;
        std::string user_name_;
        int wins_;
        int games_played_;
        int total_kills_;
        int total_damages_;
        int total_deaths_;
        int rating_;
        int highest_rating_;
    };

} // namespace game_server