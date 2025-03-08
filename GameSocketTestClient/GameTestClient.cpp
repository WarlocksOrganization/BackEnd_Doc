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

            // ���� �����͸� ������ ����
            std::string response_data;
            char buffer[1024];
            boost::system::error_code error;
            size_t bytes_read;

            // �� ������ ���: ������ ����ŷ���� ����
            socket_.non_blocking(true);

            // 10�� Ÿ�Ӿƿ�
            auto start_time = std::chrono::steady_clock::now();
            auto timeout = std::chrono::seconds(10);

            while (true) {
                // �ð� �ʰ� Ȯ��
                auto now = std::chrono::steady_clock::now();
                if (now - start_time > timeout) {
                    cout << "Response timeout\n";
                    break;
                }

                try {
                    bytes_read = socket_.read_some(boost::asio::buffer(buffer), error);

                    if (error == boost::asio::error::would_block) {
                        // �����Ͱ� ���� ����, ��� ��� �� �ٽ� �õ�
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        continue;
                    }

                    if (error) {
                        break; // �ٸ� ������ �߻��ϸ� ���� ����
                    }

                    if (bytes_read > 0) {
                        response_data.append(buffer, bytes_read);
                        cout << "Read " << bytes_read << " bytes\n";
                    }

                    // ����� �о����� Ȯ��
                    if (response_data.find("\r\n\r\n") != std::string::npos) {
                        size_t header_end = response_data.find("\r\n\r\n");
                        std::string header = response_data.substr(0, header_end);

                        // ������� Content-Length ����
                        size_t cl_pos = header.find("Content-Length: ");
                        if (cl_pos != std::string::npos) {
                            size_t cl_end = header.find("\r\n", cl_pos);
                            std::string cl_str = header.substr(cl_pos + 16, cl_end - (cl_pos + 16));
                            size_t content_length = std::stoul(cl_str);

                            // ������ �̹� ����� �������� Ȯ��
                            if (response_data.length() >= header_end + 4 + content_length) {
                                cout << "Complete response received\n";
                                break;
                            }
                        }
                        else if (header.find("Transfer-Encoding: chunked") != std::string::npos ||
                            header.find("Connection: close") != std::string::npos) {
                            // ûũ ���ڵ��̳� ���� �ݱ⸦ Ȯ�������� Ư�� ó�� �ʿ�
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

            // ����ŷ ��� ����
            socket_.non_blocking(false);

            // ���� ó��
            cout << "Total response data (" << response_data.length() << " bytes)\n";

            if (response_data.empty()) {
                return json{ {"status", "error"}, {"message", "No response from server"} };
            }

            // ����� ���� �и�
            size_t header_end = response_data.find("\r\n\r\n");
            if (header_end == std::string::npos) {
                cout << "Invalid HTTP response format\n";
                return json{ {"status", "error"}, {"message", "Invalid response format"} };
            }

            // ���� ����
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
        cout << "\n���� IP�� �Է��� �ּ��� : ";
        cin >> IP;
        cout << "\n��Ʈ ��ȣ�� �Է��� �ּ��� : ";
        cin >> Port;
        cout << "\n������ �õ��մϴ�...\n";
        GameTestClient client(IP, Port);

        try {
            if (client.connect()) {
                cout << "���� ����!\n";
                string s;
                while (1) {
                    cout << "\n�۾��� ������ �ּ���(ȸ������, �α���, ����) : ";
                    cin >> s;
                    if (s == "ȸ������") {
                        string id, pw;
                        cout << "\n����� ID�� �Է��� �ּ��� : ";
                        cin >> id;
                        cout << "\n����� ��й�ȣ�� �Է��� �ּ��� : ";
                        cin >> pw;
                        cout << "\nȸ�� ������ �õ��մϴ�...\n";
                        // Test user registration
                        client.registerUser(id, pw);
                    }
                    else if (s == "�α���") {
                        string id, pw;
                        cout << "\nID�� �Է��� �ּ��� : ";
                        cin >> id;
                        cout << "\n��й�ȣ�� �Է��� �ּ��� : ";
                        cin >> pw;
                        cout << "\n�α����� �õ��մϴ�...\n";
                        if (client.login(id, pw)) {
                            std::cout << "Login successful!" << std::endl;
                        }
                    }
                    else if (s == "����") {
                        cout << "\n���� ������ ������ �����մϴ�.\n";
                        client.disconnect();
                        break;
                    }
                    else {
                        cout << "\n�ùٸ� ���� �Է��� �ּ���\n";
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