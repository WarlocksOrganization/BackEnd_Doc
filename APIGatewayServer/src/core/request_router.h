#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <functional>

class RequestRouter {
public:
    // ����� �ڵ鷯 Ÿ�� ���� (JSON ��û�� �޾� JSON ������ ��ȯ�ϴ� �Լ�)
    using RouteHandler = std::function<nlohmann::json(const nlohmann::json&)>;

    // Ư�� �׼ǿ� ���� ����� �ڵ鷯 ���
    void registerRoute(const std::string& action, RouteHandler handler);

    // ���� ��û�� ������ �ڵ鷯�� �����
    nlohmann::json route(const nlohmann::json& request);

private:
    // �׼ǰ� �ش� �ڵ鷯 �Լ��� �����ϴ� ����� ���̺�
    std::unordered_map<std::string, RouteHandler> routes_;
};