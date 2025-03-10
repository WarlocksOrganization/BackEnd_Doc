// util/password_util.cpp
#include "password_util.h"
#include <openssl/sha.h>
#include <iomanip>
#include <sstream>
#include <spdlog/spdlog.h>

namespace game_server {

    std::string PasswordUtil::hashPassword(const std::string& password) {
        // SECURITY NOTE: Use a more secure hashing algorithm in production
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
        // Compare hashed password with hash of input password
        std::string computedHash = hashPassword(password);
        return computedHash == hashedPassword;
    }

} // namespace game_server