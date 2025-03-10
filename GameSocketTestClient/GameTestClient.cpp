// test_client.cpp
#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using boost::asio::ip::tcp;
using namespace std;

class GameTestClient {
public:
    GameTestClient(const std::string& host, int port)
        : io_context_(), socket_(io_context_), host_(host), port_(port) {
    }

    bool connect() {
        try {
            tcp::resolver resolver(io_context_);
            auto endpoints = resolver.resolve(host_, std::to_string(port_));
            boost::asio::connect(socket_, endpoints);
            std::cout << "Connected to " << host_ << ":" << port_ << std::endl;
            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "Connect error: " << e.what() << std::endl;
            return false;
        }
    }

    json sendRequest(const json& request_data) {
        try {
            // Convert JSON to string
            std::string json_str = request_data.dump();

            // Create HTTP request
            std::string http_request =
                "POST / HTTP/1.1\r\n"
                "Host: " + host_ + ":" + std::to_string(port_) + "\r\n"
                "Content-Type: application/json\r\n"
                "Content-Length: " + std::to_string(json_str.length()) + "\r\n"
                "Connection: close\r\n"
                "\r\n" +
                json_str;

            // Send request
            boost::asio::write(socket_, boost::asio::buffer(http_request));
            cout << "Send request done\n";

            // 응답 데이터를 저장할 변수
            std::string response_data;
            char buffer[1024];
            boost::system::error_code error;
            size_t bytes_read;

            // 더 간단한 방법: 소켓을 논블로킹으로 설정
            socket_.non_blocking(true);

            // 10초 타임아웃
            auto start_time = std::chrono::steady_clock::now();
            auto timeout = std::chrono::seconds(10);

            while (true) {
                // 시간 초과 확인
                auto now = std::chrono::steady_clock::now();
                if (now - start_time > timeout) {
                    cout << "Response timeout\n";
                    break;
                }

                try {
                    bytes_read = socket_.read_some(boost::asio::buffer(buffer), error);

                    if (error == boost::asio::error::would_block) {
                        // 데이터가 아직 없음, 잠시 대기 후 다시 시도
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        continue;
                    }

                    if (error) {
                        break; // 다른 오류가 발생하면 루프 종료
                    }

                    if (bytes_read > 0) {
                        response_data.append(buffer, bytes_read);
                        cout << "Read " << bytes_read << " bytes\n";
                    }

                    // 충분히 읽었는지 확인
                    if (response_data.find("\r\n\r\n") != std::string::npos) {
                        size_t header_end = response_data.find("\r\n\r\n");
                        std::string header = response_data.substr(0, header_end);

                        // 헤더에서 Content-Length 추출
                        size_t cl_pos = header.find("Content-Length: ");
                        if (cl_pos != std::string::npos) {
                            size_t cl_end = header.find("\r\n", cl_pos);
                            std::string cl_str = header.substr(cl_pos + 16, cl_end - (cl_pos + 16));
                            size_t content_length = std::stoul(cl_str);

                            // 본문이 이미 충분히 읽혔는지 확인
                            if (response_data.length() >= header_end + 4 + content_length) {
                                cout << "Complete response received\n";
                                break;
                            }
                        }
                        else if (header.find("Transfer-Encoding: chunked") != std::string::npos ||
                            header.find("Connection: close") != std::string::npos) {
                            // 청크 인코딩이나 연결 닫기를 확인했으면 특별 처리 필요
                            if (error == boost::asio::error::eof) {
                                cout << "Connection closed by server\n";
                                break;
                            }
                        }
                    }
                }
                catch (const std::exception& e) {
                    cout << "Read exception: " << e.what() << "\n";
                    break;
                }
            }

            // 논블로킹 모드 해제
            socket_.non_blocking(false);

            // 응답 처리
            cout << "Total response data (" << response_data.length() << " bytes)\n";

            if (response_data.empty()) {
                return json{ {"status", "error"}, {"message", "No response from server"} };
            }

            // 헤더와 본문 분리
            size_t header_end = response_data.find("\r\n\r\n");
            if (header_end == std::string::npos) {
                cout << "Invalid HTTP response format\n";
                return json{ {"status", "error"}, {"message", "Invalid response format"} };
            }

            // 본문 추출
            std::string body = response_data.substr(header_end + 4);
            cout << "Body content (" << body.length() << " bytes): " << body << "\n";

            try {
                return json::parse(body);
            }
            catch (const std::exception& e) {
                cout << "JSON parse error: " << e.what() << "\n";
                return json{ {"status", "error"}, {"message", "Invalid JSON in response"} };
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Request error: " << e.what() << std::endl;
            return json{ {"status", "error"}, {"message", std::string("Request failed: ") + e.what()} };
        }
    }

    bool registerUser(const std::string& username, const std::string& password) {
        json request = {
            {"action", "register"},
            {"username", username},
            {"password", password}
        };

        json response = sendRequest(request);
        std::cout << "Register response: " << response.dump(2) << std::endl;

        return response.contains("status") && response["status"] == "success";
    }

    bool login(const std::string& username, const std::string& password) {
        json request = {
            {"action", "login"},
            {"username", username},
            {"password", password}
        };

        json response = sendRequest(request);
        std::cout << "Login response: " << response.dump(2) << std::endl;

        if (response.contains("status") && response["status"] == "success") {
            user_id_ = response["user_id"];
            username_ = response["username"];
            return true;
        }
        return false;
    }

    void disconnect() {
        if (socket_.is_open()) {
            socket_.close();
            std::cout << "Disconnected from server" << std::endl;
        }
    }

    int checkMyID() {
        return user_id_;
    }

    void logout() {
        user_id_ = 0;
        username_.clear();
        return;
    }

private:
    boost::asio::io_context io_context_;
    tcp::socket socket_;
    std::string host_;
    int port_;
    int user_id_ = 0;
    std::string username_;
};

int main() {
    while (1) {
        string IP;
        int Port;
        cout << "서버 IP를 입력해 주세요 : ";
        cin >> IP;
        cout << "포트 번호를 입력해 주세요 : ";
        cin >> Port;
        cin.ignore();
        cout << "연결을 시도합니다...\n";
        GameTestClient client(IP, Port);

        try {
            if (client.connect()) {
                cout << "연결 성공!\n";
                string auth_input;
                while (1) {
                    cout << "작업을 선택해 주세요(회원가입, 로그인, 종료) : ";
                    getline(cin, auth_input);
                    if (auth_input == "회원가입") {
                        string id, pw;
                        cout << "사용할 ID를 입력해 주세요 : ";
                        cin >> id;
                        cout << "사용할 비밀번호를 입력해 주세요 : ";
                        cin >> pw;
                        cout << "회원 가입을 시도합니다...\n";
                        // Test user registration
                        client.registerUser(id, pw);
                    }
                    else if (auth_input == "로그인") {
                        string id, pw;
                        cout << "ID를 입력해 주세요 : ";
                        getline(cin, id);
                        cout << "비밀번호를 입력해 주세요 : ";
                        getline(cin, pw);
                        cout << "로그인을 시도합니다...\n";
                        if (client.login(id, pw)) {
                            std::cout << "Login successful!" << std::endl;
                            string session_input;
                            while (1) {
                                cout << "작업을 선택해 주세요(ID 확인, 방 생성, 방 참가, 로그아웃) : ";
                                getline(cin, session_input);
                                if (session_input == "ID 확인") {
                                    cout << client.checkMyID() << "\n";
                                }
                                else if (session_input == "방 생성") {

                                }
                                else if (session_input == "방 참가") {

                                }
                                else if (session_input == "로그아웃") {
                                    cout << "세션 연결을 종료합니다,\n";
                                    client.logout();
                                    break;
                                }
                                else {
                                    cout << "올바른 값을 입력해 주세요\n";
                                }
                            }
                        }
                    }
                    else if (auth_input == "종료") {
                        cout << "소켓 서버와 연결을 종료합니다.\n";
                        client.disconnect();
                        break;
                    }
                    else {
                        cout << "올바른 값을 입력해 주세요\n";
                    }
                }
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Exception: " << e.what() << std::endl;
        }
    }
    

    return 0;
}