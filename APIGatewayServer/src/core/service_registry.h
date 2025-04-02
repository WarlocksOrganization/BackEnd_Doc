#pragma once
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>
#include "service_endpoint.h"

class ServiceRegistry {
public:
    // ���ο� ���� ��������Ʈ�� ������Ʈ���� ���
    void registerService(const ServiceEndpoint& endpoint);

    // Ư�� ���� Ÿ���� ��������Ʈ ������ ��ȸ
    ServiceEndpoint getService(const std::string& service_type);

    // ���� ���ϸ� ����Ͽ� ������ ���� ��������Ʈ ����
    ServiceEndpoint getOptimalService(const std::string& service_type);

    // ������ ���¸� ������Ʈ (�ｺ üũ ��� �ݿ�)
    void updateServiceHealth(const std::string& service_name, bool is_healthy);

private:
    // ���� Ÿ�Ժ� ���� ��������Ʈ ����
    std::unordered_map<std::string, std::vector<ServiceEndpoint>> services_;

    // ���� ���� ��� ���� ���ؽ�
    std::mutex registry_mutex_;
};