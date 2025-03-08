// main.cpp
#include "core/server.h"
#include <boost/asio.hpp>
#include <iostream>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <csignal>

// �۷ι� ���� ���� (�ñ׳� �ڵ鷯��)
std::unique_ptr<game_server::Server> server;

// �ñ׳� �ڵ鷯 �Լ�
void signal_handler(int signal)
{
    spdlog::info("Received signal {}, shutting down...", signal);
    if (server) {
        server->stop();
    }
    exit(signal);
}

int main(int argc, char* argv[])
{
    try {
        // �ΰ� �ʱ�ȭ
        auto console = spdlog::stdout_color_mt("console");
        spdlog::set_default_logger(console);
        spdlog::set_level(spdlog::level::info);
        spdlog::info("Starting Game Socket Server");

        // �ñ׳� �ڵ鷯 ���
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);

        // �⺻ ����
        short port = 8080;
        std::string db_connection_string =
            "dbname=GameData user=admin password=admin host=localhost port=5432 client_encoding=UTF8";

        // Ŀ�ǵ� ���� ���� ó��
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--port" && i + 1 < argc) {
                port = std::stoi(argv[++i]);
            }
            else if (arg == "--db" && i + 1 < argc) {
                db_connection_string = argv[++i];
            }
            else if (arg == "--help") {
                std::cout << "Usage: " << argv[0] << " [options]\n"
                    << "Options:\n"
                    << "  --port PORT       Server port (default: 8080)\n"
                    << "  --db CONNSTRING   Database connection string\n"
                    << "  --help            Show this help message\n";
                return 0;
            }
        }

        // IO ���ؽ�Ʈ �� ���� ����
        boost::asio::io_context io_context;
        server = std::make_unique<game_server::Server>(
            io_context, port, db_connection_string);

        // ���� ����
        server->run();

        // IO ���ؽ�Ʈ ���� (�̺�Ʈ ����)
        spdlog::info("Server running on port {}", port);
        io_context.run();
    }
    catch (std::exception& e) {
        spdlog::error("Exception: {}", e.what());
        return 1;
    }

    return 0;
}