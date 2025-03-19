#pragma once
#include <boost/asio.hpp>
#include <nlohmann/json.hpp>
#include "service_registry.h"
#include "request_router.h"

class ApiGateway {
public:
    // ����Ʈ���� �ʱ�ȭ, IO ���ؽ�Ʈ�� ���� ��� ��Ʈ ����
    ApiGateway(boost::asio::io_context& io_context, int port);

    // ����Ʈ���� ���� ����, Ŭ���̾�Ʈ ���� ���� ����
    void start();

private:
    // �񵿱� Ŭ���̾�Ʈ ���� ����
    void do_accept();

    // Ŭ���̾�Ʈ ���� ó��
    // ���� ���� �� ��û ó�� ���� ����
    void handle_client_connection(boost::asio::ip::tcp::socket socket);

    // Boost.Asio�� IO ���ؽ�Ʈ ����
    boost::asio::io_context& io_context_;

    // Ŭ���̾�Ʈ ������ �����ϴ� Acceptor
    boost::asio::ip::tcp::acceptor acceptor_;

    // ���� ������Ʈ�� (���� �˻� �� ����)
    ServiceRegistry service_registry_;

    // ��û ����� (��û �б� �� ó��)
    RequestRouter request_router_;
};