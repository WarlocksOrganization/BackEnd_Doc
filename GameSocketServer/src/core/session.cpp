// core/session.cpp
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
        read_header();
    }

    void Session::read_header() {
        auto self = shared_from_this();

        socket_.async_read_some(
            boost::asio::buffer(buffer_),
            [this, self](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    // 간단한 HTTP 헤더 파싱 (Content-Length 찾기)
                    std::string header(buffer_.data(), length);
                    std::size_t content_length = 0;

                    // Content-Length 헤더 찾기
                    std::string cl_header = "Content-Length: ";
                    auto pos = header.find(cl_header);
                    if (pos != std::string::npos) {
                        auto end_pos = header.find("\r\n", pos);
                        if (end_pos != std::string::npos) {
                            std::string length_str = header.substr(
                                pos + cl_header.length(),
                                end_pos - (pos + cl_header.length())
                            );
                            content_length = std::stoul(length_str);
                        }
                    }

                    // 본문과 헤더 구분자 찾기
                    auto body_start = header.find("\r\n\r\n");
                    if (body_start != std::string::npos) {
                        body_start += 4; // "\r\n\r\n" 이후부터 본문 시작

                        // 이미 받은 본문 부분
                        std::string body_part = header.substr(body_start);

                        if (body_part.length() >= content_length) {
                            // 본문이 이미 완전히 수신됨
                            process_request(body_part.substr(0, content_length));
                        }
                        else {
                            // 본문의 나머지 부분 읽기 필요
                            message_ = body_part;
                            read_body(content_length - body_part.length());
                        }
                    }
                }
                else {
                    handle_error("Error reading header: " + ec.message());
                }
            });
    }

    void Session::read_body(std::size_t remaining_length) {
        auto self = shared_from_this();

        socket_.async_read_some(
            boost::asio::buffer(buffer_),
            [this, self, remaining_length](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    message_.append(buffer_.data(), length);

                    if (length >= remaining_length) {
                        // 본문 완전히 수신
                        process_request(message_);
                    }
                    else {
                        // 더 읽기 필요
                        read_body(remaining_length - length);
                    }
                }
                else {
                    handle_error("Error reading body: " + ec.message());
                }
            });
    }

    void Session::process_request(const std::string& request_data) {
        try {
            // JSON 파싱
            json request = json::parse(request_data);

            // 요청 라우팅
            std::string action = request["action"];
            std::string controller_type;

            if (action == "register" || action == "login") {
                controller_type = "auth";
            }
            else {
                // 다른 API 액션 라우팅 추가
                spdlog::warn("Unknown action: {}", action);
                json error_response = {
                    {"status", "error"},
                    {"message", "Unknown action"}
                };
                write_response(error_response.dump());
                return;
            }

            // 적절한 컨트롤러 찾기
            auto controller_it = controllers_.find(controller_type);
            if (controller_it != controllers_.end()) {
                // 컨트롤러에 요청 전달
                std::string response = controller_it->second->handleRequest(request);

                // 인증 정보 갱신 (로그인 성공 시)
                if (action == "login" && json::parse(response)["status"] == "success") {
                    json resp_json = json::parse(response);
                    user_id_ = resp_json["user_id"];
                    spdlog::info("User {} logged in", user_id_);
                }

                write_response(response);
            }
            else {
                spdlog::error("Controller not found for type: {}", controller_type);
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
        auto self = shared_from_this();

        // HTTP 응답 헤더 생성
        std::string response_header =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: " + std::to_string(response.length()) + "\r\n"
            "Connection: keep-alive\r\n"
            "\r\n";

        std::string full_response = response_header + response;

        boost::asio::async_write(
            socket_,
            boost::asio::buffer(full_response),
            [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    // 응답 후, 다음 요청 대기
                    read_header();
                }
                else {
                    handle_error("Error writing response: " + ec.message());
                }
            });
    }

    void Session::handle_error(const std::string& error_message) {
        // 에러 로깅
        spdlog::error(error_message);

        // 필요시 리소스 정리
        if (socket_.is_open()) {
            boost::system::error_code ec;
            socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
            socket_.close(ec);

            if (ec) {
                spdlog::error("Error closing socket: {}", ec.message());
            }
        }
    }

} // namespace game_server