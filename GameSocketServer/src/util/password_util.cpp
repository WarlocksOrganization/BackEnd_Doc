// util/password_util.cpp
// ��й�ȣ ��ƿ��Ƽ ���� ����
// ��й�ȣ �ؽ� �� ���� ��� ����
#include "password_util.h"
#include <openssl/sha.h>
#include <iomanip>
#include <sstream>
#include <spdlog/spdlog.h>

namespace game_server {

    std::string PasswordUtil::hashPassword(const std::string& password) {
        // ���� ����: ���� ��ǰ������ �� ������ �ؽ� �˰��� ��� �ʿ�
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(password.c_str()),
            password.length(), hash);

        // �ؽø� 16���� ���ڿ��� ��ȯ
        std::stringstream ss;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(hash[i]);
        }

        return ss.str();
    }

    bool PasswordUtil::verifyPassword(const std::string& password, const std::string& hashedPassword) {
        // �Էµ� ��й�ȣ�� �ؽÿ� ����� �ؽ� ��
        std::string computedHash = hashPassword(password);
        return computedHash == hashedPassword;
    }

} // namespace game_server