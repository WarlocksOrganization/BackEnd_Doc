// service/auth_service.cpp
// 인증 서비스 구현 파일
// 사용자 등록 및 로그인 비즈니스 로직을 처리
#include "auth_service.h"
#include "../util/password_util.h"
#include "../repository/user_repository.h"
#include <spdlog/spdlog.h>

namespace game_server {

    using json = nlohmann::json;

    // 서비스 구현체
    class AuthServiceImpl : public AuthService {
    public:
        explicit AuthServiceImpl(std::shared_ptr<UserRepository> userRepo)
            : userRepo_(userRepo) {
        }

        json registerUser(const json& request) override {
            json response;

            // 사용자명 유효성 검증
            if (!request.contains("userName") || request["userName"].get<std::string>().size() < 3 ||
                request["userName"].get<std::string>().size() > 20) {
                response["status"] = "error";
                response["message"] = "Username must be between 3 and 20 characters";
                spdlog::error("Username must be between 3 and 20 characters");
                return response;
            }

            // 비밀번호 유효성 검증
            if (!request.contains("password") || request["password"].get<std::string>().size() < 6) {
                response["status"] = "error";
                response["message"] = "Password must be at least 6 characters";
                spdlog::error("Password must be at least 6 characters");
                return response;
            }

            // 사용자명 중복 확인
            const json& userInfo = userRepo_->findByUsername(request["userName"]);
            if (userInfo["userId"] != -1) {
                response["status"] = "error";
                response["message"] = "Username already exists";
                spdlog::error("Username already exists");
                return response;
            }

            // PasswordUtil을 사용하여 비밀번호 해싱
            std::string hashedPassword = PasswordUtil::hashPassword(request["password"]);

            // 새 사용자 생성
            int userId = userRepo_->create(request["userName"], hashedPassword);
            if (userId < 0) {
                response["status"] = "error";
                response["message"] = "Failed to create user";
                spdlog::error("Failed to create user");
                return response;
            }

            // 성공 응답 생성
            response["action"] = "register";
            response["status"] = "success";
            response["message"] = "Registration successful";
            response["userId"] = userId;
            response["userName"] = request["userName"];

            spdlog::info("New user registered: {} (ID: {})", request["userName"].get<std::string>(), userId);
            return response;
        }

        json loginUser(const json& request) override {
            json response;

            // 사용자 찾기
            const json& userInfo = userRepo_->findByUsername(request["userName"]);
            if (userInfo["userId"] == -1) {
                response["status"] = "error";
                response["message"] = "Invalid username";
                return response;
            }

            // PasswordUtil을 사용하여 비밀번호 검증
            if (!PasswordUtil::verifyPassword(request["password"], userInfo["passwordHash"])) {
                response["status"] = "error";
                response["message"] = "Invalid password";
                return response;
            }

            // 로그인 시간 업데이트
            userRepo_->updateLastLogin(userInfo["userId"]);

            // 성공 응답 생성
            response["action"] = "login";
            response["status"] = "success";
            response["message"] = "Login successful";
            response["userId"] = userInfo["userId"];
            response["userName"] = userInfo["userName"];
            response["createdAt"] = userInfo["createdAt"];
            response["lastLogin"] = userInfo["lastLogin"];
            return response;
        }

    private:
        std::shared_ptr<UserRepository> userRepo_;
    };

    // 팩토리 메서드 구현
    std::unique_ptr<AuthService> AuthService::create(std::shared_ptr<UserRepository> userRepo) {
        return std::make_unique<AuthServiceImpl>(userRepo);
    }

} // namespace game_server