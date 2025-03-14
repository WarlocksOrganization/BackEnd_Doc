#pragma once
#include <string>
#include <boost/asio.hpp>
#include <nlohmann/json.hpp>

struct ServiceEndpoint {
    // 서비스의 고유 이름 (auth, room, game 등)
    std::string name;

    // 서비스의 호스트 주소 (localhost, IP 주소 등)
    std::string host;

    // 서비스의 포트 번호
    int port;

    // 서비스가 허용할 수 있는 최대 동시 연결 수
    int max_connections = 10;

    // 현재 활성화된 연결 수
    int current_connections = 0;

    // 서비스의 현재 상태 (정상 작동 중인지 여부)
    bool is_healthy = true;
};