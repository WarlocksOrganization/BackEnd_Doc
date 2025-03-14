#include "api_gateway.h"
#include <spdlog/spdlog.h>

ApiGateway::ApiGateway(boost::asio::io_context& io_context, int port)
    : io_context_(io_context),
    acceptor_(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
{
    // 서비스 초기 등록
    service_registry_.registerService({ "auth", "localhost", 8081 });
    service_registry_.registerService({ "room", "localhost", 8082 });
    service_registry_.registerService({ "game", "localhost", 8083 });

    // 라우팅 핸들러 등록
    request_router_.registerRoute("login", [](const nlohmann::json& req) {
        // 로그인 라우팅 로직
        return nlohmann::json{ {"status", "success"} };
        });

    request_router_.registerRoute("create_room", [](const nlohmann::json& req) {
        // 방 생성 라우팅 로직
        return nlohmann::json{ {"status", "success"} };
        });
}

void ApiGateway::start() {
    do_accept();
}

void ApiGateway::do_accept() {
    acceptor_.async_accept([this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
        if (!ec) {
            handle_client_connection(std::move(socket));
        }
        else {
            spdlog::error("Accept error: {}", ec.message());
        }
        do_accept();
        });
}

void ApiGateway::handle_client_connection(boost::asio::ip::tcp::socket socket) {
    // 비동기 수신 및 처리 로직
    boost::asio::async_read_until(
        socket,
        boost::asio::dynamic_buffer(std::string()),
        "\n",
        [this, socket = std::move(socket)](boost::system::error_code ec, std::size_t bytes_transferred) mutable {
            if (!ec) {
                try {
                    // JSON 파싱
                    std::string request_str(bytes_transferred, '\0');
                    socket.read_some(boost::asio::buffer(&request_str[0], bytes_transferred));

                    auto request = nlohmann::json::parse(request_str);

                    // 라우팅
                    auto response = request_router_.route(request);

                    // 서비스 선택 및 포워딩
                    auto service = service_registry_.getOptimalService(request["service"]);

                    // 응답 전송
                    boost::asio::async_write(
                        socket,
                        boost::asio::buffer(response.dump() + "\n"),
                        [](boost::system::error_code) {}
                    );
                }
                catch (const std::exception& e) {
                    spdlog::error("Request processing error: {}", e.what());
                }
            }
        }
    );
}