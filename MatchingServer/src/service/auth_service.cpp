// service/auth_service.cpp
// ���� ���� ���� ����
// ����� ��� �� �α��� ����Ͻ� ������ ó��
#include "auth_service.h"
#include "../util/password_util.h"
#include "../repository/user_repository.h"
#include <spdlog/spdlog.h>

namespace game_server {

    using json = nlohmann::json;

    // ���� ����ü
    class AuthServiceImpl : public AuthService {
    public:
        explicit AuthServiceImpl(std::shared_ptr<UserRepository> userRepo)
            : userRepo_(userRepo) {
        }

        json registerUser(const json& request) override {
            json response;

            // ����ڸ� ��ȿ�� ����
            if (!request.contains("userName") || request["userName"].get<std::string>().size() < 3 ||
                request["userName"].get<std::string>().size() > 20) {
                response["status"] = "error";
                response["message"] = "Username must be between 3 and 20 characters";
                spdlog::error("Username must be between 3 and 20 characters");
                return response;
            }

            // ��й�ȣ ��ȿ�� ����
            if (!request.contains("password") || request["password"].get<std::string>().size() < 6) {
                response["status"] = "error";
                response["message"] = "Password must be at least 6 characters";
                spdlog::error("Password must be at least 6 characters");
                return response;
            }

            // ����ڸ� �ߺ� Ȯ��
            const json& userInfo = userRepo_->findByUsername(request["userName"]);
            if (userInfo["userId"] != -1) {
                response["status"] = "error";
                response["message"] = "Username already exists";
                spdlog::error("Username already exists");
                return response;
            }

            // PasswordUtil�� ����Ͽ� ��й�ȣ �ؽ�
            std::string hashedPassword = PasswordUtil::hashPassword(request["password"]);

            // �� ����� ����
            int userId = userRepo_->create(request["userName"], hashedPassword);
            if (userId < 0) {
                response["status"] = "error";
                response["message"] = "Failed to create user";
                spdlog::error("Failed to create user");
                return response;
            }

            // ���� ���� ����
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

            // ����� ã��
            const json& userInfo = userRepo_->findByUsername(request["userName"]);
            if (userInfo["userId"] == -1) {
                response["status"] = "error";
                response["message"] = "Invalid username";
                return response;
            }

            // PasswordUtil�� ����Ͽ� ��й�ȣ ����
            if (!PasswordUtil::verifyPassword(request["password"], userInfo["passwordHash"])) {
                response["status"] = "error";
                response["message"] = "Invalid password";
                return response;
            }

            // �α��� �ð� ������Ʈ
            userRepo_->updateLastLogin(userInfo["userId"]);

            // ���� ���� ����
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

    // ���丮 �޼��� ����
    std::unique_ptr<AuthService> AuthService::create(std::shared_ptr<UserRepository> userRepo) {
        return std::make_unique<AuthServiceImpl>(userRepo);
    }

} // namespace game_server