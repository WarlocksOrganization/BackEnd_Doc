// core/session.cpp
// ?몄뀡 愿由??대옒??援ы쁽
// ?대씪?댁뼵?몄????듭떊 ?몄뀡??泥섎━?섎뒗 ?듭떖 ?뚯씪
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
        if (server_) {
            if (is_mirror_) {
                server_->removeMirrorSession(mirror_port_);
            }
            if (!token_.empty()) {
                server_->removeSession(token_, user_id_);
            }
        }
    }

    void Session::initialize() {
        // ?쒕쾭???몄뀡 ?깅줉 諛??좏겙 諛쏄린
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

    // ?몄뀡 ?쒖옉 - ?몃뱶?곗씠?щ????쒖옉
    void Session::start() {
        read_handshake();  // 癒쇱? ?몃뱶?곗씠??泥섎━
    }

    // ?몃뱶?곗씠??硫붿떆吏 泥섎━
    void Session::read_handshake() {
        auto self(shared_from_this());
        socket_.async_read_some(
            boost::asio::buffer(buffer_),
            [this, self](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    try {
                        std::string data(buffer_.data(), length);
                        json handshake = json::parse(data);

                        // 誘몃윭 ?쒕쾭 援щ텇 濡쒖쭅
                        if (handshake.contains("connectionType") &&
                            handshake["connectionType"] == "mirror" && 
                            handshake.contains("port")) {

                            // 誘몃윭 ?쒕쾭 ?몄뀡?쇰줈 ?ㅼ젙
                            is_mirror_ = true;
                            mirror_port_ = handshake["port"];
                            spdlog::info("Mirror server connection established port: {}", mirror_port_);

                            // 誘몃윭 ?쒕쾭 ?꾩슜 珥덇린??
                            server_->registerMirrorSession(shared_from_this(), handshake["port"]);
                            user_id_ = handshake["port"];

                            // ?뺤씤 ?묐떟 ?꾩넚
                            json response = {
                                {"status", "success"},
                                {"message", "Mirror server connected"}
                            };
                            write_handshake_response(response.dump());
                        }
                        else {
                            // ?쇰컲 ?대씪?댁뼵???몄뀡 珥덇린??
                            initialize();

                            // ?몃뱶?곗씠?ш? ?ㅼ젣 ?붿껌??寃쎌슦 泥섎━
                            if (handshake.contains("action")) {
                                process_request(handshake);
                            }
                            else {
                                // ?쇰컲 ?대씪?댁뼵?몄뿉寃??곌껐 ?뺤씤 硫붿떆吏 ?꾩넚
                                json response = {
                                    {"status", "success"},
                                    {"message", "Connected to server"}
                                };
                                write_handshake_response(response.dump());
                            }
                        }
                    }
                    catch (const std::exception& e) {
                        // ?몃뱶?곗씠???ㅽ뙣 泥섎━
                        spdlog::error("Handshake error: {}", e.what());
                        handle_error("Invalid handshake format");
                    }
                }
                else {
                    handle_error("Handshake reading error: " + ec.message());
                }
            });
    }

    // ?몃뱶?곗씠???묐떟 ?꾩넚 (?묐떟 ???쇰컲 硫붿떆吏 泥섎━濡??꾪솚)
    void Session::write_handshake_response(const std::string& response) {
        auto self(shared_from_this());
        boost::asio::async_write(
            socket_,
            boost::asio::buffer(response),
            [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    // ?몃뱶?곗씠???꾨즺 ???쇰컲 硫붿떆吏 泥섎━ ?쒖옉
                    read_message();
                }
                else {
                    handle_error("Handshake response writing error: " + ec.message());
                }
            });
    }

    void Session::read_message() {
        auto self(shared_from_this());

        // 鍮꾨룞湲곗쟻?쇰줈 ?곗씠???쎄린
        socket_.async_read_some(
            boost::asio::buffer(buffer_),
            [this, self](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    try {
                        // ?섏떊???곗씠?곕? 臾몄옄?대줈 蹂??
                        std::string data(buffer_.data(), length);

                        // JSON ?뚯떛
                        json request = json::parse(data);

                        // ?붿껌 泥섎━
                        process_request(request);
                    }
                    catch (const std::exception& e) {
                        // JSON ?뚯떛 ?ㅻ쪟 ???덉쇅 泥섎━
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
            // action ?꾨뱶濡??붿껌 ????뺤씤
            std::string action = request["action"];
            spdlog::debug("Action: {}", action);
            std::string controller_type;

            // 而⑦듃濡ㅻ윭 ???寃곗젙
            if (action == "register" || action == "login" || action == "SSAFYlogin" || action == "updateNickName") {
                if (user_id_) request["userId"] = user_id_;
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
            else if (action == "logout") {
                std::string logMessage = user_name_ + " had logout";
                handle_error(logMessage);
                return;
            }
            else if (action == "roomCapacity") {
                json response;
                response["action"] = "roomCapacity";
                response["status"] = "success";
                response["roomCapacity"] = server_->getRoomCapacity();
                write_response(response.dump());
                return;
            }
            else if (action == "CCU") {
                json response;
                response["action"] = "CCU";
                response["status"] = "success";
                response["roomCapacity"] = server_->getCCU();
                write_response(response.dump());
                return;
            }
            else {
                // ?????녿뒗 ?≪뀡 泥섎━
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

                if ((action == "login" || action == "SSAFYlogin") && response["status"] == "success") {
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

                    // response 媛앹껜 ?붾쾭源?濡쒓렇
                    spdlog::debug("Response content: {}", response.dump());

                    try {
                        json broad_response;
                        broad_response["action"] = "setRoom";
                        broad_response["roomId"] = response["roomId"];
                        broad_response["roomName"] = response["roomName"];
                        broad_response["maxPlayers"] = response["maxPlayers"];

                        auto mirror = server_->getMirrorSession(response["port"]);
                        if (!mirror) {
                            json error_response = {
                                {"status", "error"},
                                {"message", "Missing mirror server"}
                            };
                            spdlog::error("roomId {} has no mirror server", response["roomId"].get<int>());
                            write_response(error_response.dump());
                            return;
                        }
                        spdlog::debug("Mirror found, broadcasting message");
                        write_broadcast(broad_response.dump(), mirror);
                    }
                    catch (const std::exception& e) {
                        spdlog::error("Error in createRoom response handling: {}", e.what());
                        // ?덉쇅媛 諛쒖깮?대룄 ?먮옒 ?묐떟? ?꾩넚
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

    void Session::write_broadcast(const std::string& response, std::shared_ptr<Session> mirror) {
        boost::asio::async_write(
            mirror->socket_,
            boost::asio::buffer(response),
            [mirror](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    // ?ㅼ쓬 ?붿껌 ?湲?
                    mirror->read_message();
                }
                else {
                    mirror->handle_error("Response writing error: " + ec.message());
                }
            });
    }

    void Session::write_response(const std::string& response) {
        auto self(shared_from_this());

        // ?대씪?댁뼵?몃줈 ?묐떟 ?곗씠???꾩넚
        boost::asio::async_write(
            socket_,
            boost::asio::buffer(response),
            [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    // ?ㅼ쓬 ?붿껌 ?湲?
                    read_message();
                }
                else {
                    handle_error("Response writing error: " + ec.message());
                }
            });
    }

    void Session::handle_error(const std::string& error_message) {
        // ?ㅻ쪟 濡쒓퉭
        spdlog::info(error_message);

        // ?ъ슜?먭? 諛⑹뿉 李몄뿬 以묒씠?덈떎硫??섍?湲?泥섎━
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
            }
        }
        catch (const std::exception& e) {
            spdlog::error("Error during automatic room exit: {}", e.what());
        }

        // ?뚯폆 由ъ냼???뺣━
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
        if (response.contains("nickName")) nick_name_ = response["nickName"];
        spdlog::info("User logged in: {} (ID: {}) nickname : {}", user_name_, user_id_, nick_name_);
    }

} // namespace game_server
