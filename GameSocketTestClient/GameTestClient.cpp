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
                "Content-Type: application/json\r\n"
                "Content-Length: " + std::to_string(json_str.length()) + "\r\n"
                "\r\n" +
                json_str;

            // Send request
            boost::asio::write(socket_, boost::asio::buffer(http_request));

            // Read response
            boost::asio::streambuf response;
            boost::system::error_code error;
            boost::asio::read_until(socket_, response, "\r\n\r\n", error);

            if (error) {
                std::cerr << "Error reading headers: " << error.message() << std::endl;
                return json();
            }

            // Extract content length from headers
            std::string header_str(
                boost::asio::buffers_begin(response.data()),
                boost::asio::buffers_begin(response.data()) + response.size());

            std::size_t content_length = 0;
            std::string cl_header = "Content-Length: ";
            auto pos = header_str.find(cl_header);
            if (pos != std::string::npos) {
                auto end_pos = header_str.find("\r\n", pos);
                if (end_pos != std::string::npos) {
                    std::string length_str = header_str.substr(
                        pos + cl_header.length(),
                        end_pos - (pos + cl_header.length()));
                    content_length = std::stoul(length_str);
                }
            }

            // Read body
            response.consume(response.size());
            boost::asio::read(socket_, response,
                boost::asio::transfer_exactly(content_length), error);

            if (error && error != boost::asio::error::eof) {
                std::cerr << "Error reading body: " << error.message() << std::endl;
                return json();
            }

            // Convert response to JSON
            std::string body(
                boost::asio::buffers_begin(response.data()),
                boost::asio::buffers_begin(response.data()) + response.size());

            return json::parse(body);
        }
        catch (const std::exception& e) {
            std::cerr << "Request error: " << e.what() << std::endl;
            return json();
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

private:
    boost::asio::io_context io_context_;
    tcp::socket socket_;
    std::string host_;
    int port_;
    int user_id_ = 0;
    std::string username_;
};

int main() {
    string IP;
    int Port;
    cout << "\n서버 IP를 입력해 주세요 : ";
    cin >> IP;
    cout << "\n포트 번호를 입력해 주세요 : ";
    cin >> Port;
    cout << "\n연결을 시도합니다...\n";
    GameTestClient client(IP, Port);

    try {
        if (client.connect()) {
            cout << "연결 성공!\n";
            string s;
            while (1) {
                cout << "\n작업을 선택해 주세요(회원가입, 로그인, 종료) : ";
                cin >> s;
                if (s == "회원가입") {
                    string id, pw;
                    cout << "\n사용할 ID를 입력해 주세요 : ";
                    cin >> id;
                    cout << "\n사용할 비밀번호를 입력해 주세요 : ";
                    cin >> pw;
                    cout << "\n회원 가입을 시도합니다...\n";
                    // Test user registration
                    client.registerUser(id, pw);
                }
                else if (s == "로그인") {
                    string id, pw;
                    cout << "\nID를 입력해 주세요 : ";
                    cin >> id;
                    cout << "\n비밀번호를 입력해 주세요 : ";
                    cin >> pw;
                    cout << "\n로그인을 시도합니다...\n";
                    if (client.login(id, pw)) {
                        std::cout << "Login successful!" << std::endl;
                    }
                }
                else if (s == "종료") {
                    cout << "\n소켓 서버와 연결을 종료합니다.\n";
                    client.disconnect();
                    return 0;
                }
                else {
                    cout << "\n올바른 값을 입력해 주세요\n";
                }
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    

    return 0;
}