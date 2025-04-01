// service/auth_service.cpp
// 인증 서비스 구현 파일
// 사용자 등록 및 로그인 비즈니스 로직을 처리
#include "auth_service.h"
#include "../util/password_util.h"
#include "../repository/user_repository.h"
#include <spdlog/spdlog.h>

namespace game_server {

    using json = nlohmann::json;

    namespace {
        // 사용자 이름 유효성 검증 함수
        bool isValidUserName(const std::string& name) {
            // 빈 이름은 유효하지 않음
            if (name.empty()) {
                return false;
            }

            // 30바이트 이내인지 확인
            if (name.size() > 30) {
                return false;
            }

            // "mirror" 단어가 포함되어 있는지 확인 (대소문자 구분 없이)
            if (name.find("mirror") != std::string::npos) {
                return false;
            }

            // 이메일 형식인지 확인
            bool isEmail = (name.find('@') != std::string::npos) &&
                (name.find('.', name.find('@')) != std::string::npos);

            // 이메일이 아닌 경우 영어, 한글, 숫자, @ 문자만 포함하는지 확인
            if (!isEmail) {
                for (unsigned char c : name) {
                    // ASCII 영어와 숫자 확인
                    if ((c >= 'A' && c <= 'Z') ||
                        (c >= 'a' && c <= 'z') ||
                        (c >= '0' && c <= '9')) {
                        continue;
                    }

                    // 허용되지 않는 문자
                    return false;
                }
            }
            else {
                // 이메일인 경우 추가 검증 (간단한 이메일 형식 검사)
                // 여기서는 표준적인 이메일 문자들(영어, 숫자, 일부 특수문자) 허용
                for (unsigned char c : name) {
                    if ((c >= 'A' && c <= 'Z') ||
                        (c >= 'a' && c <= 'z') ||
                        (c >= '0' && c <= '9') ||
                        (c == '@') || (c == '.') ||
                        (c == '_') || (c == '-') || (c == '+')) {
                        continue;
                    }

                    // 이메일에 한글은 허용하지 않음 (IDN 이메일 제외)
                    return false;
                }
            }

            return true;
        }

        bool isValidNickName(const std::string& nickName) {
            // 빈 이름은 유효하지 않음
            if (nickName.empty()) {
                return false;
            }

            // 16바이트 이내인지 확인
            if (nickName.size() > 16) {
                return false;
            }

            // "mirror" 단어가 포함되어 있는지 확인 (대소문자 구분 없이)
            if (nickName.find("mirror") != std::string::npos) {
                return false;
            }

            for (unsigned char c : nickName) {
                // ASCII 영어와 숫자 확인
                if ((c >= 'A' && c <= 'Z') ||
                    (c >= 'a' && c <= 'z') ||
                    (c >= '0' && c <= '9')) {
                    continue;
                }

                // UTF-8 한글 범위 확인 (첫 바이트가 0xEA~0xED 범위)
                if ((c & 0xF0) == 0xE0) {
                    continue;
                }

                // 한글 문자의 연속 바이트 (0x80~0xBF 범위)
                if ((c & 0xC0) == 0x80) {
                    continue;
                }

                // 허용되지 않는 문자
                return false;
            }
            return true;
        }
    }

    // 서비스 구현체
    class AuthServiceImpl : public AuthService {
    public:
        explicit AuthServiceImpl(std::shared_ptr<UserRepository> userRepo)
            : userRepo_(userRepo) {
        }

        json registerUser(const json& request) override {
            json response;

            // 사용자명 유효성 검증
            if (!request.contains("userName") || !request.contains("password")) {
                response["status"] = "error";
                response["message"] = "요청 JSON에 userName 또는 password가 없습니다";
                spdlog::error("요청 JSON에 userName 또는 password가 없습니다");
                return response;
            }

            if (!isValidUserName(request["userName"])) {
                response["status"] = "error";
                response["message"] = "사용자 이름이 유효하지 않습니다";
                spdlog::error("사용자 이름이 유효하지 않습니다");
                return response;
            }

            // 비밀번호 유효성 검증
            if (request["password"].get<std::string>().size() < 6) {
                response["status"] = "error";
                response["message"] = "비밀번호는 최소 6자 이상이어야 합니다";
                spdlog::error("비밀번호는 최소 6자 이상이어야 합니다");
                return response;
            }

            // 사용자명 중복 확인
            const json& userInfo = userRepo_->findByUsername(request["userName"]);
            if (userInfo["userId"] != -1) {
                response["status"] = "error";
                response["message"] = "이미 존재하는 사용자 이름입니다";
                spdlog::error("이미 존재하는 사용자 이름입니다");
                return response;
            }

            // PasswordUtil을 사용하여 비밀번호 해싱
            std::string hashedPassword = PasswordUtil::hashPassword(request["password"]);

            // 새 사용자 생성
            int userId = userRepo_->create(request["userName"], hashedPassword);
            if (userId < 0) {
                response["status"] = "error";
                response["message"] = "사용자 생성에 실패했습니다";
                spdlog::error("사용자 생성에 실패했습니다");
                return response;
            }

            // 성공 응답 생성
            response["action"] = "register";
            response["status"] = "success";
            response["message"] = "회원가입에 성공했습니다";
            response["userId"] = userId;
            response["userName"] = request["userName"];

            spdlog::info("새 사용자 등록: {} (ID: {})", request["userName"].get<std::string>(), userId);
            return response;
        }

        json loginUser(const json& request) override {
            json response;

            // 사용자명 유효성 검증
            if (!request.contains("userName") || !request.contains("password")) {
                response["status"] = "error";
                response["message"] = "요청 JSON에 userName 또는 password가 없습니다";
                spdlog::error("요청 JSON에 userName 또는 password가 없습니다");
                return response;
            }

            // 사용자 찾기
            const json& userInfo = userRepo_->findByUsername(request["userName"]);
            if (userInfo["userId"] == -1) {
                response["status"] = "error";
                response["message"] = "유효하지 않은 사용자 이름입니다";
                return response;
            }

            // PasswordUtil을 사용하여 비밀번호 검증
            if (!PasswordUtil::verifyPassword(request["password"], userInfo["passwordHash"])) {
                response["status"] = "error";
                response["message"] = "유효하지 않은 비밀번호입니다";
                return response;
            }

            // 로그인 시간 업데이트
            userRepo_->updateLastLogin(userInfo["userId"]);

            // 성공 응답 생성
            response["action"] = "login";
            response["status"] = "success";
            response["message"] = "로그인에 성공했습니다";
            response["userId"] = userInfo["userId"];
            response["userName"] = userInfo["userName"];
            response["nickName"] = userInfo["nickName"];
            response["createdAt"] = userInfo["createdAt"];
            response["lastLogin"] = userInfo["lastLogin"];
            return response;
        }

        json registerCheckAndLogin(const nlohmann::json& request) {
            json response;

            // 사용자명 유효성 검증
            if (!request.contains("userName") || !request.contains("password")) {
                response["status"] = "error";
                response["message"] = "요청 JSON에 userName 또는 password가 없습니다";
                spdlog::error("요청 JSON에 userName 또는 password가 없습니다");
                return response;
            }

            // 사용자 찾기
            const json& userInfo = userRepo_->findByUsername(request["userName"]);
            int userId = -1;
            if (userInfo["userId"] == -1) {
                // PasswordUtil을 사용하여 비밀번호 해싱
                std::string hashedPassword = PasswordUtil::hashPassword(request["password"]);

                // 새 사용자 생성
                userId = userRepo_->create(request["userName"], hashedPassword);
                if (userId < 0) {
                    response["status"] = "error";
                    response["message"] = "사용자 생성에 실패했습니다";
                    spdlog::error("사용자 생성에 실패했습니다");
                    return response;
                }
            }

            // 로그인 시간 업데이트
            userRepo_->updateLastLogin(userId);

            // 성공 응답 생성
            response["action"] = "login";
            response["status"] = "success";
            response["message"] = "로그인에 성공했습니다";
            response["userId"] = userInfo["userId"];
            response["userName"] = userInfo["userName"];
            response["nickName"] = userInfo["nickName"];
            response["createdAt"] = userInfo["createdAt"];
            response["lastLogin"] = userInfo["lastLogin"];
            return response;
        }

        json updateNickName(const nlohmann::json& request) {
            json response;

            // 사용자명 유효성 검증
            if (!request.contains("userId") || !request.contains("nickName")) {
                response["status"] = "error";
                response["message"] = "요청 JSON에 userId 또는 nickName이 없습니다";
                spdlog::error("요청 JSON에 userId 또는 nickName이 없습니다");
                return response;
            }

            if (!isValidNickName(request["nickName"])) {
                response["status"] = "error";
                response["message"] = "유효하지 않은 닉네임 형식입니다";
                spdlog::error("유효하지 않은 닉네임 형식입니다");
                return response;
            }

            // 사용자 찾기
            if (!userRepo_->updateUserNickName(request["userId"], request["nickName"])) {
                response["status"] = "error";
                response["message"] = "사용자 닉네임 업데이트에 실패했습니다";
                spdlog::error("사용자 닉네임 업데이트에 실패했습니다");
                return response;
            }

            // 성공 응답 생성
            response["action"] = "updateNickName";
            response["status"] = "success";
            response["message"] = "닉네임 업데이트에 성공했습니다";
            spdlog::info("사용자 ID: {}가 닉네임을 {}(으)로 설정했습니다", request["userId"].get<int>(), request["nickName"].get<std::string>());
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