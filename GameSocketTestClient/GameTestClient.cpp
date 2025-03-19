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

    json registerUser(const std::string& username, const std::string& password) {
        json request = { {"action", "register"}, {"username", username}, {"password", password} };
        json response = sendRequest(request);
        return response;
    }

    json login(const std::string& username, const std::string& password) {
        json request = { {"action", "login"}, {"username", username}, {"password", password} };
        json response = sendRequest(request);

        if (response.contains("status") && response["status"] == "success") {
            user_info_ = response;
        }
        return response;
    }

    json createRoom(const std::string& roomname, const int& maxplayers) {
        json request = { {"action", "create_room"}, {"room_name", roomname}, {"maxplayes", maxplayers} };
        json response = sendRequest(request);
        return response;
    }

    void disconnect() {
        if (socket_.is_open()) {
            socket_.close();
            cout << "서버 연결 종료\n";
        }
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
        string IP, id, pw, auth_input, session_input, roomName, maxPlayers, roomId;
        int Port;

        //cout << "서버 IP를 입력하세요: ";
        //cin >> IP;
        //cout << "포트 번호를 입력하세요: ";
        //cin >> Port;
        //cin.ignore();

        while (true) {
            cout << "서버에 연결 중...\n";
            auto client = std::make_unique<GameClient>("127.0.0.1", 8080);

            try {
                if (!client->connect()) continue;

                while (true) {
                    cout << "[메뉴] 회원가입 / 로그인 / 종료\n입력: ";
                    getline(cin, auth_input);

                    if (auth_input == "회원가입") {
                        cout << "ID : ";
                        cin >> id;
                        cout << "PW : ";
                        cin >> pw;
                        json response = client->registerUser(id, pw);
                        if (response["status"] == "error") {
                            cout << response["message"] << "\n";
                            continue;
                        }
                        cout << "회원가입 성공!\n";
                    }
                    else if (auth_input == "로그인") {
                        cout << "ID : ";
                        cin >> id;
                        cout << "PW : ";
                        cin >> pw;
                        json response = client->login(id, pw);
                        if (response["status"] == "error") {
                            cout << response["message"] << "\n";
                            continue;
                        }
                        cout << "로그인 성공!\n";
                        while (true) {
                            cout << "[메뉴] 방 생성 / 방 참가 / 방 목록 / 로그아웃\n입력: ";
                            getline(cin, session_input);
                            if (session_input == "방 생성") {
                                cout << "방 이름 : ";
                                cin >> roomName;
                                cout << "최대 플레이어 : ";
                                cin >> maxPlayers;
                                json response = client->createRoom(roomName, stoi(maxPlayers));
                                if (response["status"] == "error") {
                                    cout << response["message"] << "\n";
                                    continue;
                                }
                                cout << "방 생성 성공! 방 번호 : " << response["room_id"] << ", 방 이름 : " << response["room_name"] << "\n";
                                while (true) {

                                }
                            }
                        }
                    }
                    else if (auth_input == "종료") {
                        client->disconnect();
                    }
                    else cout << "알 수 없는 명령어 입니다.\n";
                }
            }
            catch (const std::exception& e) {
                cerr << "오류 발생: " << e.what() << endl;
            }
        }
    }
}
