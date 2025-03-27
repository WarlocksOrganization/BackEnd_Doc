// core/session.cpp
// 세션 관리 클래스 구현
// 클라이언트와의 통신 세션을 처리하는 핵심 파일
#include "session.h"
#include "server.h"
#include <iostream>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace game_server {

    using json = nlohmann::json;

    Session::Session(boost::asio::ip::tcp::socket socket,
        std::map<std::string, std::shared_ptr<Controller>>& controllers,
        Server* server)
        : socket_(std::move(socket)),
        controllers_(controllers),
        user_id_(0),
        server_(server),
        last_activity_time_(std::chrono::steady_clock::now())
    {
        spdlog::info("New session created from {}:{}",
            socket_.remote_endpoint().address().to_string(),
            socket_.remote_endpoint().port());
    }

    Session::~Session() {
        if (server_ && !token_.empty()) {
            server_->removeSession(token_, user_id_);
        }
    }

    void Session::initialize() {
        // 서버에 세션 등록 및 토큰 받기
        token_ = server_->registerSession(shared_from_this());
        spdlog::info("Session initialized with token {}", token_);
    }

    void Session::handlePing() {
        last_activity_time_ = std::chrono::steady_clock::now();

        json response = {
            {"action", "refreshSession"},
            {"status", "success"},
            {"message", "pong"},
            {"sessionToken", token_}
        };

        write_response(response.dump());
        spdlog::debug("Ping received, session {} updated", token_);
    }

    bool Session::isActive(std::chrono::seconds timeout) const {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_activity_time_);
        return elapsed < timeout;
    }

    const std::string& Session::getToken() const {
        return token_;
    }

    // 세션 시작 - 핸드셰이크부터 시작
    void Session::start() {
        read_handshake();  // 먼저 핸드셰이크 처리
    }

    // 핸드셰이크 메시지 처리
    void Session::read_handshake() {
        auto self(shared_from_this());
        socket_.async_read_some(
            boost::asio::buffer(buffer_),
            [this, self](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    try {
                        std::string data(buffer_.data(), length);
                        json handshake = json::parse(data);

                        // 미러 서버 구분 로직
                        if (handshake.contains("connectionType") &&
                            handshake["connectionType"] == "mirror" && 
                            handshake.contains("port")) {

                            // 미러 서버 세션으로 설정
                            is_mirror_ = true;
                            spdlog::info("Mirror server connection established");

                            // 미러 서버 전용 초기화
                            server_->mirrors_[handshake["port"]] = shared_from_this();

                            // 확인 응답 전송
                            json response = {
                                {"status", "success"},
                                {"message", "Mirror server connected"}
                            };
                            write_handshake_response(response.dump());
                        }
                        else {
                            // 일반 클라이언트 세션 초기화
                            initialize();

                            // 핸드셰이크가 실제 요청인 경우 처리
                            if (handshake.contains("action")) {
                                process_request(handshake);
                            }
                            else {
                                // 일반 클라이언트에게 연결 확인 메시지 전송
                                json response = {
                                    {"status", "success"},
                                    {"message", "Connected to server"}
                                };
                                write_handshake_response(response.dump());
                            }
                        }
                    }
                    catch (const std::exception& e) {
                        // 핸드셰이크 실패 처리
                        spdlog::error("Handshake error: {}", e.what());
                        handle_error("Invalid handshake format");
                    }
                }
                else {
                    handle_error("Handshake reading error: " + ec.message());
                }
            });
    }

    // 핸드셰이크 응답 전송 (응답 후 일반 메시지 처리로 전환)
    void Session::write_handshake_response(const std::string& response) {
        auto self(shared_from_this());
        boost::asio::async_write(
            socket_,
            boost::asio::buffer(response),
            [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    // 핸드셰이크 완료 후 일반 메시지 처리 시작
                    read_message();
                }
                else {
                    handle_error("Handshake response writing error: " + ec.message());
                }
            });
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
            spdlog::debug("Processing request...");
            // action 필드로 요청 타입 확인
            std::string action = request["action"];
            spdlog::debug("Action: {}", action);
            std::string controller_type;

            // 컨트롤러 타입 결정
            if (action == "register" || action == "login" || action == "SSAFYlogin") {
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
            else if (action == "alivePing") {
                handlePing();
                return;
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

            auto controller_it = controllers_.find(controller_type);
            if (controller_it != controllers_.end()) {
                spdlog::debug("Controller found: {}", controller_type);
                json response = controller_it->second->handleRequest(request);
                spdlog::debug("Controller response received");

                if (action == "login" && response["status"] == "success") {
                    spdlog::debug("Processing login response");
                    if (server_->checkAlreadyLogin(response["userId"].get<int>())) {
                        spdlog::error("user ID : {} is already login", response["userId"].get<int>());
                        json error_response = {
                            {"status", "error"},
                            {"message", "Already login user"}
                        };
                        write_response(error_response.dump());
                        return;
                    }

                    init_current_user(response);
                    std::string token = server_->registerSession(shared_from_this());
                    token_ = token;
                    response["sessionToken"] = token;
                }
                else if (action == "createRoom" &&  response["status"] == "success") {
                    spdlog::debug("Processing createRoom response");

                    // response 객체 디버깅 로그
                    spdlog::debug("Response content: {}", response.dump());

                    try {
                        json broad_response;
                        broad_response["action"] = "setRoom";
                        broad_response["roomId"] = response["roomId"];
                        broad_response["roomName"] = response["roomName"];
                        broad_response["maxPlayers"] = response["maxPlayers"];

                        int port = response["port"];
                        if (server_->mirrors_.count(port)) {
                            spdlog::debug("Mirror found, broadcasting message");
                            write_broadcast(broad_response.dump(), server_->mirrors_[port]);
                        }
                    }
                    catch (const std::exception& e) {
                        spdlog::error("Error in createRoom response handling: {}", e.what());
                        // 예외가 발생해도 원래 응답은 전송
                    }
                }

                spdlog::debug("Sending response to client");
                write_response(response.dump());
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
            spdlog::error("Error in process_request: {}", e.what());
            if (request.is_object() && request.contains("action")) {
                spdlog::error("Failed action: {}", request["action"].get<std::string>());
            }
            json error_response = {
                {"status", "error"},
                {"message", "Invalid request format"}
            };
            write_response(error_response.dump());
        }
    }

    void Session::write_broadcast(const std::string& response, std::shared_ptr<Session>& mirror) {
        boost::asio::async_write(
            mirror->socket_,
            boost::asio::buffer(response),
            [mirror](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    // 다음 요청 대기
                    mirror->read_message();
                }
                else {
                    mirror->handle_error("Response writing error: " + ec.message());
                }
            });
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
        // 오류 로깅
        spdlog::error(error_message);

        // 사용자가 방에 참여 중이었다면 나가기 처리
        try {
            auto controller_it = controllers_.find("room");
            if (controller_it != controllers_.end() && user_id_ > 0) {
                spdlog::debug("Attempting automatic room exit for user {}", user_id_);

                json temp = {
                    {"action", "exitRoom"},
                    {"userId", user_id_}
                };

                json response = controller_it->second->handleRequest(temp);

                if (response.contains("status") && response["status"] == "success") {
                    spdlog::info("User {} automatically exited from room on session termination", user_id_);
                }
                else {
                    spdlog::warn("Failed to exit room for user {} on session termination", user_id_);
                }
            }
        }
        catch (const std::exception& e) {
            spdlog::error("Error during automatic room exit: {}", e.what());
        }

        // 소켓 리소스 정리
        if (socket_.is_open()) {
            boost::system::error_code ec;
            socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
            if (ec) {
                spdlog::error("Socket shutdown error: {}", ec.message());
            }

            socket_.close(ec);
            if (ec) {
                spdlog::error("Socket close error: {}", ec.message());
            }
            else {
                spdlog::info("Socket closed successfully");
            }
        }
    }

    int Session::getUserId() {
        return user_id_;
    }

    void Session::setToken(const std::string& token) {
        token_ = token;
    }

    void Session::init_current_user(const json& response) {
        if (response.contains("userId")) user_id_ = response["userId"];
        if (response.contains("userName")) user_name_ = response["userName"];
        spdlog::info("User logged in: {} (ID: {})", user_name_, user_id_);
    }

} // namespace game_server