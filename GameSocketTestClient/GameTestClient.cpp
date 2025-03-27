#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using boost::asio::ip::tcp;
using namespace std;

class GameClient {
public:
    GameClient(const std::string& host, int port)
        : io_context_(), socket_(io_context_), host_(host), port_(port), user_id_(0) {
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
            // userId가 설정되었고 요청에 없으면 추가
            json modified_request = request_data;
            if (user_id_ > 0 && !request_data.contains("userId")) {
                // 로그인/회원가입/핸드셰이크 요청이 아닌 경우에만 userId 추가
                std::string action = request_data.contains("action") ?
                    request_data["action"].get<std::string>() : "";

                if (action != "login" && action != "register" && action != "SSAFYlogin" &&
                    action != "" && !request_data.contains("connectionType")) {
                    modified_request["userId"] = user_id_;
                }
            }

            std::string request = modified_request.dump();
            boost::asio::write(socket_, boost::asio::buffer(request));
            cout << "요청 전송 완료: " << request << endl;

            // 응답 받기
            boost::asio::streambuf response_buffer;
            boost::system::error_code ec;
            size_t bytes_transferred = 0;

            // 응답 데이터 읽기
            bytes_transferred = boost::asio::read_until(socket_, response_buffer, "}", ec);

            if (ec && ec != boost::asio::error::eof) {
                throw boost::system::system_error(ec);
            }

            // 응답 데이터 처리
            std::string response_str((std::istreambuf_iterator<char>(&response_buffer)),
                std::istreambuf_iterator<char>());

            if (response_str.empty()) {
                return json{ {"status", "error"}, {"message", "서버 응답 없음"} };
            }

            // JSON 객체가 불완전할 경우 나머지 데이터 읽기 시도
            if (response_str.find_last_of('}') != response_str.length() - 1) {
                // 추가 데이터 읽기
                boost::asio::streambuf additional_buffer;
                bytes_transferred = boost::asio::read(socket_, additional_buffer,
                    boost::asio::transfer_at_least(1), ec);

                std::string additional_str((std::istreambuf_iterator<char>(&additional_buffer)),
                    std::istreambuf_iterator<char>());

                response_str += additional_str;
            }

            cout << "받은 응답: " << response_str << endl;

            json response = json::parse(response_str);

            // 로그인 응답에서 userId 저장
            if (response.contains("status") && response["status"] == "success") {
                if (response.contains("action")) {
                    std::string action = response["action"];
                    if ((action == "login" || action == "register") && response.contains("userId")) {
                        user_id_ = response["userId"];
                        cout << "로그인 성공: 사용자 ID " << user_id_ << endl;
                    }
                }
            }

            return response;
        }
        catch (const json::parse_error& e) {
            cerr << "JSON 파싱 오류: " << e.what() << endl;
            return json{ {"status", "error"}, {"message", std::string("JSON 파싱 오류: ") + e.what()} };
        }
        catch (const std::exception& e) {
            cerr << "요청 오류: " << e.what() << endl;
            return json{ {"status", "error"}, {"message", e.what()} };
        }
    }

    void disconnect() {
        if (socket_.is_open()) {
            socket_.close();
            cout << "서버 연결 종료\n";
        }
    }

    int getUserId() const {
        return user_id_;
    }

private:
    boost::asio::io_context io_context_;
    tcp::socket socket_;
    std::string host_;
    int port_;
    int user_id_; // 로그인 후 사용자 ID 저장
};

int main() {
    try {
        //GameClient client("j12a509.p.ssafy.io", 8080);
        GameClient client("127.0.0.1", 8080);

        if (!client.connect()) {
            cerr << "서버 연결 실패\n";
            return 1;
        }

        // 핸드셰이크
        json handshake_request = {};
        json handshake_response = client.sendRequest(handshake_request);
        cout << "핸드셰이크 응답:\n" << handshake_response.dump(2) << endl << endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));

        int choice;
        bool running = true;

        while (running) {
            cout << "\n===== 게임 서버 API 테스트 =====\n";
            cout << "1. 회원가입\n";
            cout << "2. 로그인\n";
            cout << "3. SSAFY 로그인\n";
            cout << "4. 닉네임 변경\n";
            cout << "5. 방 목록 조회\n";
            cout << "6. 방 생성\n";
            cout << "7. 방 참가\n";
            cout << "8. 방 나가기\n";
            cout << "9. 게임 시작\n";
            cout << "10. 게임 종료\n";
            cout << "11. Ping 보내기\n";
            cout << "0. 종료\n";
            cout << "선택: ";
            cin >> choice;

            // 입력 버퍼 비우기
            cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

            json request;
            json response;

            switch (choice) {
            case 1: {
                // 회원가입
                string username, password;
                cout << "사용자 이름: ";
                getline(cin, username);
                cout << "비밀번호: ";
                getline(cin, password);

                request = {
                    {"action", "register"},
                    {"userName", username},
                    {"password", password}
                };
                response = client.sendRequest(request);
                cout << "회원가입 응답:\n" << response.dump(2) << endl;
                break;
            }
            case 2: {
                // 로그인
                string username, password;
                cout << "사용자 이름: ";
                getline(cin, username);
                cout << "비밀번호: ";
                getline(cin, password);

                request = {
                    {"action", "login"},
                    {"userName", username},
                    {"password", password}
                };
                response = client.sendRequest(request);
                cout << "로그인 응답:\n" << response.dump(2) << endl;
                break;
            }
            case 3: {
                // SSAFY 로그인
                string username, password;
                cout << "사용자 이름: ";
                getline(cin, username);
                cout << "비밀번호: ";
                getline(cin, password);

                request = {
                    {"action", "SSAFYlogin"},
                    {"userName", username},
                    {"password", password}
                };
                response = client.sendRequest(request);
                cout << "SSAFY 로그인 응답:\n" << response.dump(2) << endl;
                break;
            }
            case 4: {
                // 닉네임 변경
                string nickname;
                cout << "새 닉네임: ";
                getline(cin, nickname);

                request = {
                    {"action", "updateNickName"},
                    {"nickName", nickname}
                };
                response = client.sendRequest(request);
                cout << "닉네임 변경 응답:\n" << response.dump(2) << endl;
                break;
            }
            case 5: {
                // 방 목록 조회
                request = {
                    {"action", "listRooms"}
                };
                response = client.sendRequest(request);
                cout << "방 목록 응답:\n" << response.dump(2) << endl;
                break;
            }
            case 6: {
                // 방 생성
                string roomName;
                int maxPlayers;
                cout << "방 이름: ";
                getline(cin, roomName);
                cout << "최대 플레이어 수 (2-8): ";
                cin >> maxPlayers;
                cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

                request = {
                    {"action", "createRoom"},
                    {"roomName", roomName},
                    {"maxPlayers", maxPlayers}
                };
                response = client.sendRequest(request);
                cout << "방 생성 응답:\n" << response.dump(2) << endl;
                break;
            }
            case 7: {
                // 방 참가
                int roomId;
                cout << "참가할 방 ID: ";
                cin >> roomId;
                cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

                request = {
                    {"action", "joinRoom"},
                    {"roomId", roomId}
                };
                response = client.sendRequest(request);
                cout << "방 참가 응답:\n" << response.dump(2) << endl;
                break;
            }
            case 8: {
                // 방 나가기
                request = {
                    {"action", "exitRoom"}
                };
                response = client.sendRequest(request);
                cout << "방 나가기 응답:\n" << response.dump(2) << endl;
                break;
            }
            case 9: {
                // 게임 시작
                int roomId, mapId;
                cout << "방 ID: ";
                cin >> roomId;
                cout << "맵 ID: ";
                cin >> mapId;
                cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

                request = {
                    {"action", "gameStart"},
                    {"roomId", roomId},
                    {"mapId", mapId}
                };
                response = client.sendRequest(request);
                cout << "게임 시작 응답:\n" << response.dump(2) << endl;
                break;
            }
            case 10: {
                // 게임 종료
                int gameId;
                cout << "게임 ID: ";
                cin >> gameId;
                cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

                request = {
                    {"action", "gameEnd"},
                    {"gameId", gameId}
                };
                response = client.sendRequest(request);
                cout << "게임 종료 응답:\n" << response.dump(2) << endl;
                break;
            }
            case 11: {
                // Ping 보내기
                request = {
                    {"action", "alivePing"}
                };
                response = client.sendRequest(request);
                cout << "Ping 응답:\n" << response.dump(2) << endl;
                break;
            }
            case 0:
                running = false;
                break;
            default:
                cout << "잘못된 선택입니다. 다시 선택해주세요.\n";
                break;
            }

            // 각 요청 사이에 잠시 대기
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        client.disconnect();
    }
    catch (const std::exception& e) {
        cerr << "오류 발생: " << e.what() << endl;
    }

    return 0;
}