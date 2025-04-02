﻿// main.cpp
// 프로그램 진입점 및 서버 실행 파일
#include "core/server.h"
#include <boost/asio.hpp>
#include <iostream>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <csignal>

// 시그널 핸들러용 전역 서버 변수
std::unique_ptr<game_server::Server> server;
const std::string VERSION = "0.5.0";

// 시그널 핸들러 함수
void signal_handler(int signal)
{
    spdlog::info("시그널 받음 {}, 서버 종료...", signal);
    if (server) {
        server->stop();
    }
    std::exit(0);
    exit(signal);
}

int main(int argc, char* argv[])
{
    try {
        // 로거 초기화
        auto console = spdlog::stdout_color_mt("console");
        spdlog::set_default_logger(console);
        spdlog::set_level(spdlog::level::info);
        spdlog::info("서버 버전 : {}", VERSION);

        // 시그널 핸들러 등록
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);

        // 기본 설정
        short port = 8080;
        std::string db_connection_string =
            "dbname=gamedata user=admin password=admin host=localhost port=5432 client_encoding=UTF8";

        // IO 컨텍스트 및 서버 생성
        boost::asio::io_context io_context;
        server = std::make_unique<game_server::Server>(
            io_context, port, db_connection_string, VERSION);

        // 서버 실행
        server->run();

        // IO 컨텍스트 실행 (이벤트 루프)
        spdlog::info("서버 시작, 포트 : {}", port);
        io_context.run();
    }
    catch (std::exception& e) {
        spdlog::error("서버 설정 중 예외 발생: {}", e.what());
        return 1;
    }

    return 0;
}

