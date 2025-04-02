#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <nlohmann/json.hpp>
#include <mutex>
#include <deque>
#include <condition_variable>

using json = nlohmann::json;
using boost::asio::ip::tcp;
using namespace std;

class GameClient {
public:
    GameClient(const std::string& host, int port)
        : io_context_(),
        socket_(io_context_),
        host_(host),
        port_(port),
        user_id_(0),
        running_(false),
        io_thread_(),
        ping_thread_() {
    }

    ~GameClient() {
        stop();
    }

    bool connect() {
        try {
            tcp::resolver resolver(io_context_);
            auto endpoints = resolver.resolve(host_, std::to_string(port_));
            boost::asio::connect(socket_, endpoints);
            cout << "서버 연결 성공: " << host_ << ":" << port_ << endl;

            // 비동기 읽기 시작
            running_ = true;
            io_thread_ = std::thread([this]() { this->runIoContext(); });

            // alivePing 스레드 시작
            ping_thread_ = std::thread([this]() { this->runPingThread(); });

            return true;
        }
        catch (const std::exception& e) {
            cerr << "연결 오류: " << e.what() << endl;
            return false;
        }
    }

    void stop() {
        running_ = false;

        if (io_thread_.joinable()) {
            io_thread_.join();
        }

        if (ping_thread_.joinable()) {
            ping_thread_.join();
        }

        disconnect();
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

            // 동기식 쓰기 작업 시 뮤텍스로 보호
            {
                std::lock_guard<std::mutex> lock(write_mutex_);
                boost::asio::write(socket_, boost::asio::buffer(request));
            }

            // ping이 아닌 경우에만 요청 로그 출력
            if (!request_data.contains("action") || request_data["action"] != "alivePing") {
                cout << "요청 전송 완료: " << request << endl;
            }

            // 응답 대기 (동기식)
            std::unique_lock<std::mutex> lock(response_mutex_);
            response_cv_.wait_for(lock, std::chrono::seconds(5), [this] { return !response_queue_.empty(); });

            if (!response_queue_.empty()) {
                json response = response_queue_.front();
                response_queue_.pop_front();
                return response;
            }

            return json{ {"status", "error"}, {"message", "응답 대기 시간 초과"} };
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

    std::atomic<bool> running_;
    std::thread io_thread_;
    std::thread ping_thread_;

    std::mutex write_mutex_;
    std::mutex response_mutex_;
    std::condition_variable response_cv_;
    std::deque<json> response_queue_;

    void runIoContext() {
        try {
            // 비동기 읽기 작업 시작
            startAsyncRead();

            // io_context 실행
            io_context_.run();
        }
        catch (const std::exception& e) {
            cerr << "IO 스레드 오류: " << e.what() << endl;
        }
    }

    void startAsyncRead() {
        auto response_buffer = std::make_shared<boost::asio::streambuf>();

        // 비동기 read_until 시작
        boost::asio::async_read_until(
            socket_,
            *response_buffer,
            '}',
            [this, response_buffer](const boost::system::error_code& ec, std::size_t bytes_transferred) {
                this->handleRead(ec, bytes_transferred, response_buffer);
            }
        );
    }

    void handleRead(const boost::system::error_code& ec, std::size_t bytes_transferred,
        std::shared_ptr<boost::asio::streambuf> response_buffer) {
        if (!ec) {
            try {
                // 응답 데이터 처리
                std::string response_str((std::istreambuf_iterator<char>(response_buffer.get())),
                    std::istreambuf_iterator<char>());

                // 불완전한 JSON 처리
                if (response_str.find_last_of('}') != response_str.length() - 1) {
                    // 추가 데이터 읽기는 이 예제에서는 생략 (실제 구현 시 추가 필요)
                }

                // ping 응답이 아닌 경우에만 로깅
                if (response_str.find("\"action\":\"refreshSession\"") == std::string::npos) {
                    cout << "받은 응답: " << response_str << endl;
                }

                json response = json::parse(response_str);

                // 로그인 응답에서 userId 저장
                if (response.contains("status") && response["status"] == "success") {
                    if (response.contains("action")) {
                        std::string action = response["action"];
                        if ((action == "login" || action == "register" || action == "SSAFYlogin") &&
                            response.contains("userId")) {
                            user_id_ = response["userId"];
                            cout << "로그인 성공: 사용자 ID " << user_id_ << endl;
                        }
                    }
                }

                // 응답 큐에 추가
                {
                    std::lock_guard<std::mutex> lock(response_mutex_);
                    response_queue_.push_back(response);
                }
                response_cv_.notify_one();

                // 다음 읽기 작업 시작
                startAsyncRead();
            }
            catch (const json::parse_error& e) {
                cerr << "JSON 파싱 오류: " << e.what() << endl;

                // 다음 읽기 작업 시작
                startAsyncRead();
            }
        }
        else if (ec != boost::asio::error::operation_aborted) {
            cerr << "읽기 오류: " << ec.message() << endl;

            if (running_) {
                // 오류가 발생했지만 아직 실행 중이면 다시 읽기 시도
                startAsyncRead();
            }
        }
    }

    void runPingThread() {
        while (running_) {
            try {
                // 10초 대기
                std::this_thread::sleep_for(std::chrono::seconds(10));

                if (!running_) break;

                // alivePing 요청 보내기
                json ping_request = {
                    {"action", "alivePing"}
                };

                // 로그 없이 ping 요청 송신
                std::string request = ping_request.dump();
                {
                    std::lock_guard<std::mutex> lock(write_mutex_);
                    boost::asio::write(socket_, boost::asio::buffer(request));
                }
            }
            catch (const std::exception& e) {
                // Ping 스레드에서 발생한 예외는 무시하지만 로깅
                cerr << "Ping 스레드 오류: " << e.what() << endl;
            }
        }
    }
};

int main() {
    try {
        GameClient client("j12a509.p.ssafy.io", 8080);
        //GameClient client("127.0.0.1", 8080);

        if (!client.connect()) {
            cerr << "서버 연결 실패\n";
            return 1;
        }

        // 핸드셰이크
        //json handshake_request = {
        //    {"connectionType", "mirror"},
        //    {"port", 40000}
        //};
        json handshake_request = {
            {"connectionType", "client"}
        };
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
                // Ping 수동 보내기
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

        // 클라이언트 정리 (모든 스레드 종료)
        client.stop();
    }
    catch (const std::exception& e) {
        cerr << "오류 발생: " << e.what() << endl;
    }

    return 0;
}