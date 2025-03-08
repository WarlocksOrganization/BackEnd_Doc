// util/password_util.h
#pragma once
#include <string>

namespace game_server {

    // ��й�ȣ �ؽ� ���� ��ƿ��Ƽ �Լ�
    class PasswordUtil {
    public:
        // ��й�ȣ �ؽ� �Լ�
        static std::string hashPassword(const std::string& password);

        // ��й�ȣ ���� �Լ� (�ʿ��� ���)
        static bool verifyPassword(const std::string& password, const std::string& hashedPassword);
    };

} // namespace game_server