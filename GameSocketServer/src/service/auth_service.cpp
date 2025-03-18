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
            if (request["user_name"].empty() || request["user_name"].size() < 3 ||
                request["user_name"].size() > 20) {
                response["status"] = "error";
                response["message"] = "Username must be between 3 and 20 characters";
                return response;
            }

            // ��й�ȣ ��ȿ�� ����
            if (request["password"].empty() || request["password"].size() < 6) {
                response["status"] = "error";
                response["message"] = "Password must be at least 6 characters";
                return response;
            }

            // ����ڸ� �ߺ� Ȯ��
            const json& userInfo = userRepo_->findByUsername(request["user_name"]);
            if (userInfo["user_id"] != -1) {
                response["status"] = "error";
                response["message"] = "Username already exists";
                return response;
            }

            // PasswordUtil�� ����Ͽ� ��й�ȣ �ؽ�
            std::string hashedPassword = PasswordUtil::hashPassword(request["password"]);

            // �� ����� ����
            int user_id = userRepo_->create(request["user_name"], hashedPassword);
            if (user_id < 0) {
                response["status"] = "error";
                response["message"] = "Failed to create user";
                return response;
            }

            // ���� ���� ����
            response["status"] = "success";
            response["message"] = "Registration successful";
            response["user_id"] = user_id;
            response["user_name"] = request["user_name"];

            spdlog::info("New user registered: {} (ID: {})", request["user_name"], user_id);
            return response;
        }

        json loginUser(const json& request) override {
            json response;

            // ����� ã��
            const json& userInfo = userRepo_->findByUsername(request["user_name"]);
            if (userInfo["user_id"] == -1) {
                response["status"] = "error";
                response["message"] = "Invalid username";
                return response;
            }

            // PasswordUtil�� ����Ͽ� ��й�ȣ ����
            if (!PasswordUtil::verifyPassword(request["password"], userInfo["password_hash"])) {
                response["status"] = "error";
                response["message"] = "Invalid password";
                return response;
            }

            // �α��� �ð� ������Ʈ
            userRepo_->updateLastLogin(userInfo["user_id"]);

            // ���� ���� ����
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

            spdlog::info("User logged in: {} (ID: {})", userInfo["user_name"], userInfo["user_id"]);
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