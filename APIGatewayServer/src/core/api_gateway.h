#pragma once
#include <boost/asio.hpp>
#include <nlohmann/json.hpp>
#include "service_registry.h"
#include "request_router.h"

class ApiGateway {
public:
    // 게이트웨이 초기화, IO 컨텍스트와 수신 대기 포트 설정
    ApiGateway(boost::asio::io_context& io_context, int port);

    // 게이트웨이 서버 시작, 클라이언트 연결 수락 시작
    void start();

private:
    // 비동기 클라이언트 연결 수락
    void do_accept();

    // 클라이언트 연결 처리
    // 소켓 연결 후 요청 처리 로직 수행
    void handle_client_connection(boost::asio::ip::tcp::socket socket);

    // Boost.Asio의 IO 컨텍스트 참조
    boost::asio::io_context& io_context_;

    // 클라이언트 연결을 수신하는 Acceptor
    boost::asio::ip::tcp::acceptor acceptor_;

    // 서비스 레지스트리 (서비스 검색 및 관리)
    ServiceRegistry service_registry_;

    // 요청 라우터 (요청 분기 및 처리)
    RequestRouter request_router_;
};