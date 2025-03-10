// service/auth_service.cpp
#include "auth_service.h"
#include "../repository/user_repository.h"
#include "../util/password_util.h"
#include <spdlog/spdlog.h>

namespace game_server {

    // Service implementation
    class AuthServiceImpl : public AuthService {
    public:
        explicit AuthServiceImpl(std::shared_ptr<UserRepository> userRepo)
            : userRepo_(userRepo) {
        }

        RegisterResponse registerUser(const RegisterRequest& request) override {
            RegisterResponse response;

            // Validate username
            if (request.username.empty() || request.username.length() < 3 ||
                request.username.length() > 20) {
                response.success = false;
                response.message = "Username must be between 3 and 20 characters";
                return response;
            }

            // Validate password
            if (request.password.empty() || request.password.length() < 6) {
                response.success = false;
                response.message = "Password must be at least 6 characters";
                return response;
            }

            // Check for username duplication
            auto existingUser = userRepo_->findByUsername(request.username);
            if (existingUser) {
                response.success = false;
                response.message = "Username already exists";
                return response;
            }

            // Hash password using PasswordUtil
            std::string hashedPassword = PasswordUtil::hashPassword(request.password);

            // Create new user
            int userId = userRepo_->create(request.username, hashedPassword);
            if (userId < 0) {
                response.success = false;
                response.message = "Failed to create user";
                return response;
            }

            // Create success response
            response.success = true;
            response.message = "Registration successful";
            response.userId = userId;
            response.username = request.username;

            spdlog::info("New user registered: {} (ID: {})", request.username, userId);
            return response;
        }

        LoginResponse loginUser(const LoginRequest& request) override {
            LoginResponse response;

            // Find user
            auto user = userRepo_->findByUsername(request.username);
            if (!user) {
                response.success = false;
                response.message = "Invalid username or password";
                return response;
            }

            // Verify password using PasswordUtil
            if (!PasswordUtil::verifyPassword(request.password, user->passwordHash)) {
                response.success = false;
                response.message = "Invalid username or password";
                return response;
            }

            // Update login time
            userRepo_->updateLastLogin(user->userId);

            // Create success response
            response.success = true;
            response.message = "Login successful";
            response.userId = user->userId;
            response.username = user->username;
            response.rating = user->rating;
            response.totalGames = user->totalGames;
            response.totalWins = user->totalWins;

            spdlog::info("User logged in: {} (ID: {})", request.username, user->userId);
            return response;
        }

    private:
        std::shared_ptr<UserRepository> userRepo_;
    };

    // Factory method implementation
    std::unique_ptr<AuthService> AuthService::create(std::shared_ptr<UserRepository> userRepo) {
        return std::make_unique<AuthServiceImpl>(userRepo);
    }

} // namespace game_server