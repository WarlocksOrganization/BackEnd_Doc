// core/session.cpp
// 세션 관리 클래스 구현
// 클라이언트와의 통신 세션을 처리하는 핵심 파일
#include "session.h"
#include <iostream>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace game_server {

    using json = nlohmann::json;

    Session::Session(boost::asio::ip::tcp::socket socket,
        std::map<std::string, std::shared_ptr<Controller>>& controllers)
        : socket_(std::move(socket)),
        controllers_(controllers),
        user_id_(0)
    {
        spdlog::info("New session created from {}:{}",
            socket_.remote_endpoint().address().to_string(),
            socket_.remote_endpoint().port());
    }

    void Session::start() {
        read_message();
    }

    void Session::read_message() {
        auto self(shared_from_this());

        // 비동기적으로 데이터 읽기
        socket_.async_read_some(
            boost::asio::buffer(buffer_),
            [this, self](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    try {
                        // 수신된 데이터를 문자열로 변환
                        std::string data(buffer_.data(), length);

                        // JSON 파싱
                        json request = json::parse(data);

                        // 요청 처리
                        process_request(request);
                    }
                    catch (const std::exception& e) {
                        // JSON 파싱 오류 등 예외 처리
                        spdlog::error("Error processing request data: {}", e.what());
                        json error_response = {
                            {"status", "error"},
                            {"message", "Invalid request format"}
                        };
                        write_response(error_response.dump());
                    }
                }
                else {
                    handle_error("Message reading error: " + ec.message());
                }
            });
    }

    void Session::process_request(json& request) {
        try {
            // action 필드로 요청 타입 확인
            std::string action = request["action"];
            std::string controller_type;

            // 컨트롤러 타입 결정
            if (action == "register" || action == "login") {
                controller_type = "auth";
            }
            else if (action == "createRoom" || action == "joinRoom" || action == "exitRoom" || action == "listRooms") {
                if (user_id_ == 0) {
                    json error_response = {
                        {"status", "error"},
                        {"message", "Authentication required"}
                    };
                    write_response(error_response.dump());
                    return;
                }

                request["userId"] = user_id_;
                controller_type = "room";
            }
            else if (action == "gameStart" || action == "gameEnd") {
                if (user_id_ == 0) {
                    json error_response = {
                        {"status", "error"},
                        {"message", "Authentication required"}
                    };
                    write_response(error_response.dump());
                    return;
                }

                request["userId"] = user_id_;
                controller_type = "game";
            }
            else {
                // 알 수 없는 액션 처리
                spdlog::warn("Unknown action: {}", action);
                json error_response = {
                    {"status", "error"},
                    {"message", "Unknown action"}
                };
                write_response(error_response.dump());
                return;
            }

            // 인증 컨트롤러 처리
            auto controller_it = controllers_.find(controller_type);
            if (controller_it != controllers_.end()) {
                // 요청을 컨트롤러로 전달
                json response = controller_it->second->handleRequest(request);
                if (action == "login" && response["status"] == "success") {
                    init_current_user(response);
                }
                write_response(response);
            }
            else {
                spdlog::error("Controller not found: {}", controller_type);
                json error_response = {
                    {"status", "error"},
                    {"message", "Internal server error"}
                };
                write_response(error_response.dump());
            }
        }
        catch (const std::exception& e) {
            spdlog::error("Error processing request: {}", e.what());
            json error_response = {
                {"status", "error"},
                {"message", "Invalid request format"}
            };
            write_response(error_response.dump());
        }
    }

    void Session::write_response(const std::string& response) {
        auto self(shared_from_this());

        // 클라이언트로 응답 데이터 전송
        boost::asio::async_write(
            socket_,
            boost::asio::buffer(response),
            [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    // 다음 요청 대기
                    read_message();
                }
                else {
                    handle_error("Response writing error: " + ec.message());
                }
            });
    }

    void Session::handle_error(const std::string& error_message) {
        // 오류 로깅 및 리소스 정리
        spdlog::error(error_message);

        // 필요한 경우 리소스 정리
        if (socket_.is_open()) {
            boost::system::error_code ec;
            socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
            socket_.close(ec);
            spdlog::error("Socket closing error: {}", error_message);

            if (ec) {
                spdlog::error("Socket closing error: {}", ec.message());
            }
        }
    }

    void Session::init_current_user(const json& response) {
        if (response.contains("userId")) user_id_ = response["userId"];
        if (response.contains("userName")) user_name_ = response["userName"];

        spdlog::info("User logged in: {} (ID: {})", user_name_, user_id_);
    }

} // namespace game_server