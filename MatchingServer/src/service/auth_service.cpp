// service/auth_service.cpp
// ���� ���� ���� ����
// ����� ��� �� �α��� ����Ͻ� ������ ó��
#include "auth_service.h"
#include "../util/password_util.h"
#include "../repository/user_repository.h"
#include <spdlog/spdlog.h>

namespace game_server {

    using json = nlohmann::json;

    namespace {
        // ����� �̸� ��ȿ�� ���� �Լ�
        bool isValidUserName(const std::string& name) {
            // �� �̸��� ��ȿ���� ����
            if (name.empty()) {
                return false;
            }

            // 30����Ʈ �̳����� Ȯ��
            if (name.size() > 30) {
                return false;
            }

            // "mirror" �ܾ ���ԵǾ� �ִ��� Ȯ�� (��ҹ��� ���� ����)
            if (name.find("mirror") != std::string::npos) {
                return false;
            }

            // �̸��� �������� Ȯ��
            bool isEmail = (name.find('@') != std::string::npos) &&
                (name.find('.', name.find('@')) != std::string::npos);

            // �̸����� �ƴ� ��� ����, �ѱ�, ����, @ ���ڸ� �����ϴ��� Ȯ��
            if (!isEmail) {
                for (unsigned char c : name) {
                    // ASCII ����� ���� Ȯ��
                    if ((c >= 'A' && c <= 'Z') ||
                        (c >= 'a' && c <= 'z') ||
                        (c >= '0' && c <= '9')) {
                        continue;
                    }

                    // ������ �ʴ� ����
                    return false;
                }
            }
            else {
                // �̸����� ��� �߰� ���� (������ �̸��� ���� �˻�)
                // ���⼭�� ǥ������ �̸��� ���ڵ�(����, ����, �Ϻ� Ư������) ���
                for (unsigned char c : name) {
                    if ((c >= 'A' && c <= 'Z') ||
                        (c >= 'a' && c <= 'z') ||
                        (c >= '0' && c <= '9') ||
                        (c == '@') || (c == '.') ||
                        (c == '_') || (c == '-') || (c == '+')) {
                        continue;
                    }

                    // �̸��Ͽ� �ѱ��� ������� ���� (IDN �̸��� ����)
                    return false;
                }
            }

            return true;
        }

        bool isValidNickName(const std::string& nickName) {
            // �� �̸��� ��ȿ���� ����
            if (nickName.empty()) {
                return false;
            }

            // 16����Ʈ �̳����� Ȯ��
            if (nickName.size() > 16) {
                return false;
            }

            // "mirror" �ܾ ���ԵǾ� �ִ��� Ȯ�� (��ҹ��� ���� ����)
            if (nickName.find("mirror") != std::string::npos) {
                return false;
            }

            for (unsigned char c : nickName) {
                // ASCII ����� ���� Ȯ��
                if ((c >= 'A' && c <= 'Z') ||
                    (c >= 'a' && c <= 'z') ||
                    (c >= '0' && c <= '9')) {
                    continue;
                }

                // UTF-8 �ѱ� ���� Ȯ�� (ù ����Ʈ�� 0xEA~0xED ����)
                if ((c & 0xF0) == 0xE0) {
                    continue;
                }

                // �ѱ� ������ ���� ����Ʈ (0x80~0xBF ����)
                if ((c & 0xC0) == 0x80) {
                    continue;
                }

                // ������ �ʴ� ����
                return false;
            }
            return true;
        }
    }

    // ���� ����ü
    class AuthServiceImpl : public AuthService {
    public:
        explicit AuthServiceImpl(std::shared_ptr<UserRepository> userRepo)
            : userRepo_(userRepo) {
        }

        json registerUser(const json& request) override {
            json response;

            // ����ڸ� ��ȿ�� ����
            if (!request.contains("userName") || !request.contains("password")) {
                response["status"] = "error";
                response["message"] = "The request json doesn't have userName or password";
                spdlog::error("The request json doesn't have userName or password");
                return response;
            }

            if (!isValidUserName(request["userName"])) {
                response["status"] = "error";
                response["message"] = "user name is unvalid";
                spdlog::error("user name is unvalid");
                return response;
            }

            // ��й�ȣ ��ȿ�� ����
            if (request["password"].get<std::string>().size() < 6) {
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

            // ����ڸ� ��ȿ�� ����
            if (!request.contains("userName") || !request.contains("password")) {
                response["status"] = "error";
                response["message"] = "The request json doesn't have userName or password";
                spdlog::error("The request json doesn't have userName or password");
                return response;
            }

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
            response["nickName"] = userInfo["nickName"];
            response["createdAt"] = userInfo["createdAt"];
            response["lastLogin"] = userInfo["lastLogin"];
            return response;
        }

        json registerCheckAndLogin(const nlohmann::json& request) {
            json response;

            // ����ڸ� ��ȿ�� ����
            if (!request.contains("userName") || !request.contains("password")) {
                response["status"] = "error";
                response["message"] = "The request json doesn't have userName or password";
                spdlog::error("The request json doesn't have userName or password");
                return response;
            }

            // ����� ã��
            const json& userInfo = userRepo_->findByUsername(request["userName"]);
            int userId = -1;
            if (userInfo["userId"] == -1) {
                // PasswordUtil�� ����Ͽ� ��й�ȣ �ؽ�
                std::string hashedPassword = PasswordUtil::hashPassword(request["password"]);

                // �� ����� ����
                userId = userRepo_->create(request["userName"], hashedPassword);
                if (userId < 0) {
                    response["status"] = "error";
                    response["message"] = "Failed to create user";
                    spdlog::error("Failed to create user");
                    return response;
                }
            }

            // �α��� �ð� ������Ʈ
            userRepo_->updateLastLogin(userId);

            // ���� ���� ����
            response["action"] = "login";
            response["status"] = "success";
            response["message"] = "Login successful";
            response["userId"] = userInfo["userId"];
            response["userName"] = userInfo["userName"];
            response["nickName"] = userInfo["nickName"];
            response["createdAt"] = userInfo["createdAt"];
            response["lastLogin"] = userInfo["lastLogin"];
            return response;
        }

        json updateNickName(const nlohmann::json& request) {
            json response;

            // ����ڸ� ��ȿ�� ����
            if (!request.contains("userId") || !request.contains("nickName")) {
                response["status"] = "error";
                response["message"] = "The request json doesn't have userId or nickName";
                spdlog::error("The request json doesn't have userId or nickName");
                return response;
            }

            if (!isValidNickName(request["nickName"])) {
                response["status"] = "error";
                response["message"] = "Invalid nickname type";
                spdlog::error("Invalid nickname type");
                return response;
            }

            // ����� ã��
            if (!userRepo_->updateUserNickName(request["userId"], request["nickName"])) {
                response["status"] = "error";
                response["message"] = "Fail to update user nickname";
                spdlog::error("Fail to update user nickname");
                return response;
            }

            // ���� ���� ����
            response["action"] = "updateNickName";
            response["status"] = "success";
            response["message"] = "Update nickname successful";
            spdlog::info("User ID : {} set nickname to {}", request["userId"].get<int>(), request["nickName"].get<std::string>());
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