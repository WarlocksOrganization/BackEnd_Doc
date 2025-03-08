// util/password_util.h
#pragma once
#include <string>

namespace game_server {

    // 비밀번호 해싱 관련 유틸리티 함수
    class PasswordUtil {
    public:
        // 비밀번호 해싱 함수
        static std::string hashPassword(const std::string& password);

        // 비밀번호 검증 함수 (필요한 경우)
        static bool verifyPassword(const std::string& password, const std::string& hashedPassword);
    };

} // namespace game_server