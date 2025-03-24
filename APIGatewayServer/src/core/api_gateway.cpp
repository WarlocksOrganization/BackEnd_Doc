#include "api_gateway.h"
#include <spdlog/spdlog.h>

ApiGateway::ApiGateway(boost::asio::io_context& io_context, int port)
    : io_context_(io_context),
    acceptor_(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
{
    // ���� �ʱ� ���
    service_registry_.registerService({ "auth", "localhost", 8081 });
    service_registry_.registerService({ "room", "localhost", 8082 });
    service_registry_.registerService({ "game", "localhost", 8083 });

    // ����� �ڵ鷯 ���
    request_router_.registerRoute("login", [](const nlohmann::json& req) {
        // �α��� ����� ����
        return nlohmann::json{ {"status", "success"} };
        });

    request_router_.registerRoute("create_room", [](const nlohmann::json& req) {
        // �� ���� ����� ����
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
    // �񵿱� ���� �� ó�� ����
    boost::asio::async_read_until(
        socket,
        boost::asio::dynamic_buffer(std::string()),
        "\n",
        [this, socket = std::move(socket)](boost::system::error_code ec, std::size_t bytes_transferred) mutable {
            if (!ec) {
                try {
                    // JSON �Ľ�
                    std::string request_str(bytes_transferred, '\0');
                    socket.read_some(boost::asio::buffer(&request_str[0], bytes_transferred));

                    auto request = nlohmann::json::parse(request_str);

                    // �����
                    auto response = request_router_.route(request);

                    // ���� ���� �� ������
                    auto service = service_registry_.getOptimalService(request["service"]);

                    // ���� ����
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