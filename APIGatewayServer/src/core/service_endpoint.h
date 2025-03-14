#pragma once
#include <string>
#include <boost/asio.hpp>
#include <nlohmann/json.hpp>

struct ServiceEndpoint {
    // ������ ���� �̸� (auth, room, game ��)
    std::string name;

    // ������ ȣ��Ʈ �ּ� (localhost, IP �ּ� ��)
    std::string host;

    // ������ ��Ʈ ��ȣ
    int port;

    // ���񽺰� ����� �� �ִ� �ִ� ���� ���� ��
    int max_connections = 10;

    // ���� Ȱ��ȭ�� ���� ��
    int current_connections = 0;

    // ������ ���� ���� (���� �۵� ������ ����)
    bool is_healthy = true;
};