// util/password_util.cpp
#include "password_util.h"
#include <openssl/sha.h>
#include <iomanip>
#include <sstream>
#include <spdlog/spdlog.h>

namespace game_server {

    std::string PasswordUtil::hashPassword(const std::string& password) {
        // SECURITY NOTE: 프로덕션에서는 더 안전한 해싱 알고리즘 사용 필요
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(password.c_str()),
            password.length(), hash);

        std::stringstream ss;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(hash[i]);
        }

        return ss.str();
    }

    bool PasswordUtil::verifyPassword(const std::string& password, const std::string& hashedPassword) {
        // 해시된 비밀번호와 입력된 비밀번호의 해시를 비교
        std::string computedHash = hashPassword(password);
        return computedHash == hashedPassword;
    }

} // namespace game_server