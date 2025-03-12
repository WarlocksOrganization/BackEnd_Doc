#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>
#include <memory>

using json = nlohmann::json;
using boost::asio::ip::tcp;
using namespace std;

class GameClient {
public:
    GameClient(const std::string& host, int port)
        : io_context_(), socket_(io_context_), host_(host), port_(port) {
    }

    bool connect() {
        try {
            tcp::resolver resolver(io_context_);
            auto endpoints = resolver.resolve(host_, std::to_string(port_));
            boost::asio::connect(socket_, endpoints);
            cout << "서버 연결 성공: " << host_ << ":" << port_ << endl;
            return true;
        }
        catch (const std::exception& e) {
            cerr << "연결 오류: " << e.what() << endl;
            return false;
        }
    }

    json sendRequest(const json& request_data) {
        try {
            std::string request = request_data.dump();
            boost::asio::write(socket_, boost::asio::buffer(request));
            cout << "요청 전송 완료\n";

            // 응답 데이터 저장
            boost::asio::streambuf response_buffer;
            boost::asio::read_until(socket_, response_buffer, "\0");

            // 스트림 버퍼를 문자열로 변환
            std::istream response_stream(&response_buffer);
            std::string response_data((std::istreambuf_iterator<char>(response_stream)), std::istreambuf_iterator<char>());

            if (response_data.empty()) {
                return json{ {"status", "error"}, {"message", "서버 응답 없음"} };
            }

            return json::parse(response_data);

        }
        catch (const std::exception& e) {
            cerr << "요청 오류: " << e.what() << endl;
            return json{ {"status", "error"}, {"message", "요청 실패"} };
        }
    }

    bool registerUser(const std::string& username, const std::string& password) {
        json request = { {"action", "register"}, {"username", username}, {"password", password} };
        json response = sendRequest(request);
        cout << "회원가입 응답: " << response.dump(2) << endl;
        return response.contains("status") && response["status"] == "success";
    }

    bool login(const std::string& username, const std::string& password) {
        json request = { {"action", "login"}, {"username", username}, {"password", password} };
        json response = sendRequest(request);

        if (response.contains("status") && response["status"] == "success") {
            user_info_ = response;
            return true;
        }
        return false;
    }

    void disconnect() {
        if (socket_.is_open()) {
            socket_.close();
            cout << "서버 연결 종료\n";
        }
    }

    int getUserId() {
        return user_info_["user_id"];
    }

    void logout() {
        user_info_.clear();
        cout << "로그아웃 완료\n";
    }

private:
    boost::asio::io_context io_context_;
    tcp::socket socket_;
    std::string host_;
    int port_;
    json user_info_;
};

int main() {
    while (true) {
        string IP, id, pw, auth_input, session_input;
        int Port;

        //cout << "서버 IP를 입력하세요: ";
        //cin >> IP;
        //cout << "포트 번호를 입력하세요: ";
        //cin >> Port;
        //cin.ignore();

        cout << "서버에 연결 중...\n";
        auto client = std::make_unique<GameClient>("127.0.0.1", 8080);

        try {
            if (!client->connect()) continue;

            while (true) {
                cout << "\n[메뉴] 회원가입 / 로그인 / 종료\n입력: ";
                getline(cin, auth_input);

                if (auth_input == "회원가입") {
                    cout << "사용할 ID: ";
                    getline(cin, id);
                    cout << "비밀번호: ";
                    getline(cin, pw);
                    if (client->registerUser(id, pw)) {
                        cout << "회원가입 성공!\n";
                    }
                }
                else if (auth_input == "로그인") {
                    cout << "ID: ";
                    getline(cin, id);
                    cout << "비밀번호: ";
                    getline(cin, pw);

                    if (client->login(id, pw)) {
                        cout << "로그인 성공!\n";

                        while (true) {
                            cout << "\n[메뉴] 내 정보 / 방 생성 / 방 참가 / 로그아웃\n입력: ";
                            getline(cin, session_input);

                            if (session_input == "내 정보") {
                                cout << "내 ID: " << client->getUserId() << "\n";
                            }
                            else if (session_input == "방 생성") {
                                cout << "방 생성 기능 준비 중...\n";
                            }
                            else if (session_input == "방 참가") {
                                cout << "방 참가 기능 준비 중...\n";
                            }
                            else if (session_input == "로그아웃") {
                                client->logout();
                                break;
                            }
                            else {
                                cout << "잘못된 입력입니다.\n";
                            }
                        }
                    }
                }
                else if (auth_input == "종료") {
                    client->disconnect();
                    return 0;
                }
                else {
                    cout << "올바른 입력이 아닙니다.\n";
                }
            }
        }
        catch (const std::exception& e) {
            cerr << "오류 발생: " << e.what() << endl;
        }
    }
}
