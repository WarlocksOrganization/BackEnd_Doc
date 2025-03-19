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
            spdlog::info("request name : {}", request["user_name"].get<std::string>());
            spdlog::info("request PW : {}", request["password"].get<std::string>());
            spdlog::info("request name size : {}", request["user_name"].get<std::string>().size());
            spdlog::info("request PW size : {}", request["password"].get<std::string>().size());
            spdlog::info("request name size : {}", request["user_name"].get<std::string>().length());
            spdlog::info("request PW size : {}", request["password"].get<std::string>().length());

            // 사용자명 유효성 검증
            if (!request.contains("user_name") || request["user_name"].get<std::string>().size() < 3 ||
                request["user_name"].get<std::string>().size() > 20) {
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
            const json& userInfo = userRepo_->findByUsername(request["user_name"]);
            if (userInfo["user_id"] != -1) {
                response["status"] = "error";
                response["message"] = "Username already exists";
                spdlog::error("Username already exists");
                return response;
            }

            // PasswordUtil을 사용하여 비밀번호 해싱
            std::string hashedPassword = PasswordUtil::hashPassword(request["password"]);

            // 새 사용자 생성
            int user_id = userRepo_->create(request["user_name"], hashedPassword);
            if (user_id < 0) {
                response["status"] = "error";
                response["message"] = "Failed to create user";
                spdlog::error("Failed to create user");
                return response;
            }

            // 성공 응답 생성
            response["status"] = "success";
            response["message"] = "Registration successful";
            response["user_id"] = user_id;
            response["user_name"] = request["user_name"];

            spdlog::info("New user registered: {} (ID: {})", request["user_name"].get<std::string>(), user_id);
            return response;
        }

        json loginUser(const json& request) override {
            json response;

            // 사용자 찾기
            const json& userInfo = userRepo_->findByUsername(request["user_name"]);
            if (userInfo["user_id"] == -1) {
                response["status"] = "error";
                response["message"] = "Invalid username";
                return response;
            }

            // PasswordUtil을 사용하여 비밀번호 검증
            if (!PasswordUtil::verifyPassword(request["password"], userInfo["password_hash"])) {
                response["status"] = "error";
                response["message"] = "Invalid password";
                return response;
            }

            // 로그인 시간 업데이트
            userRepo_->updateLastLogin(userInfo["user_id"]);

            // 성공 응답 생성
            response["status"] = "success";
            response["message"] = "Login successful";
            response["user_id"] = userInfo["user_id"];
            response["user_name"] = userInfo["user_name"];
            response["wins"] = userInfo["wins"];
            response["games_played"] = userInfo["games_played"];
            response["total_kills"] = userInfo["total_kills"];
            response["total_damages"] = userInfo["total_damages"];
            response["total_deaths"] = userInfo["total_deaths"];
            response["rating"] = userInfo["rating"];
            response["highest_rating"] = userInfo["highest_rating"];
            response["created_at"] = userInfo["created_at"];
            response["last_login"] = userInfo["last_login"];

            spdlog::info("User logged in: {} (ID: {})", userInfo["user_name"].get<std::string>(), userInfo["user_id"].get<int>());
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