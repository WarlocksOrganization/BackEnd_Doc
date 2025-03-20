// main.cpp
// 프로그램 진입점 및 서버 실행 파일
#include "core/server.h"
#include <boost/asio.hpp>
#include <iostream>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <csignal>

// 시그널 핸들러용 전역 서버 변수
std::unique_ptr<game_server::Server> server;

// 시그널 핸들러 함수
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
        // 로거 초기화
        auto console = spdlog::stdout_color_mt("console");
        spdlog::set_default_logger(console);
        spdlog::set_level(spdlog::level::info);
        spdlog::info("게임 서버 시작");

        // 시그널 핸들러 등록
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);

        // 기본 설정
        short port = 8080;
        std::string db_connection_string =
            "dbname=gamedata user=admin password=admin host=localhost port=5432 client_encoding=UTF8";

        // 명령줄 인수 처리
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--port" && i + 1 < argc) {
                port = std::stoi(argv[++i]);
            }
            else if (arg == "--db" && i + 1 < argc) {
                db_connection_string = argv[++i];
            }
            else if (arg == "--help") {
                std::cout << "사용법: " << argv[0] << " [옵션]\n"
                    << "옵션:\n"
                    << "  --port PORT       서버 포트 (기본값: 8080)\n"
                    << "  --db CONNSTRING   데이터베이스 연결 문자열\n"
                    << "  --help            도움말 표시\n";
                return 0;
            }
        }

        // IO 컨텍스트 및 서버 생성
        boost::asio::io_context io_context;
        server = std::make_unique<game_server::Server>(
            io_context, port, db_connection_string);

        // 서버 실행
        server->run();

        // IO 컨텍스트 실행 (이벤트 루프)
        spdlog::info("Server running on port {}", port);
        io_context.run();
    }
    catch (std::exception& e) {
        spdlog::error("Exception: {}", e.what());
        return 1;
    }

    return 0;
}