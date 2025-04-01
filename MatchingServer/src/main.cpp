// main.cpp
// ���α׷� ������ �� ���� ���� ����
#include "core/server.h"
#include <boost/asio.hpp>
#include <iostream>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <csignal>

// �ñ׳� �ڵ鷯�� ���� ���� ����
std::unique_ptr<game_server::Server> server;

// �ñ׳� �ڵ鷯 �Լ�
void signal_handler(int signal)
{
    spdlog::info("Received signal {}, shutting down...", signal);
    if (server) {
        server->stop();
    }
    std::exit(0);
    exit(signal);
}

int main(int argc, char* argv[])
{
    try {
        setlocale(LC_ALL, "ko_KR.UTF-8");
        // �ΰ� �ʱ�ȭ
        auto console = spdlog::stdout_color_mt("console");
        spdlog::set_default_logger(console);
        spdlog::set_level(spdlog::level::info);
        spdlog::info("SmashUp server v1.0.2");

        // �ñ׳� �ڵ鷯 ���
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);

        // �⺻ ����
        short port = 8080;
        std::string db_connection_string =
            "dbname=gamedata user=admin password=admin host=localhost port=5432 client_encoding=UTF8";

        // IO ���ؽ�Ʈ �� ���� ����
        boost::asio::io_context io_context;
        server = std::make_unique<game_server::Server>(
            io_context, port, db_connection_string);

        // ���� ����
        server->run();

        // IO ���ؽ�Ʈ ���� (�̺�Ʈ ����)
        spdlog::info("���� ���� ��Ʈ : {}", port);
        io_context.run();
    }
    catch (std::exception& e) {
        spdlog::error("Exception: {}", e.what());
        return 1;
    }

    return 0;
}
