#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <nlohmann/json.hpp>

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
            cout << "요청 전송 완료: " << request << endl;

            // 응답 받기
            boost::asio::streambuf response_buffer;
            boost::system::error_code ec;
            size_t bytes_read = boost::asio::read(socket_, response_buffer, ec);

            if (ec && ec != boost::asio::error::eof) {
                throw boost::system::system_error(ec);
            }

            // 응답 데이터 처리
            std::istream response_stream(&response_buffer);
            std::string response_str;
            std::getline(response_stream, response_str, '\0');

            if (response_str.empty()) {
                return json{ {"status", "error"}, {"message", "서버 응답 없음"} };
            }

            return json::parse(response_str);
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

private:
    boost::asio::io_context io_context_;
    tcp::socket socket_;
    std::string host_;
    int port_;
};

int main() {
    try {
        GameClient client("j12a509.p.ssafy.io", 8080);

        if (!client.connect()) {
            cerr << "서버 연결 실패\n";
            return 1;
        }

        // 회원가입 요청
        json register_request = {
            {"action", "register"},
            {"userName", "zzzzz955@gmail.com"},
            {"password", "password123"}
        };

        json register_response = client.sendRequest(register_request);
        cout << "회원가입 응답:\n" << register_response.dump(2) << endl << endl;

        // 로그인 요청
        json login_request = {
            {"action", "login"},
            {"userName", "zzzzz955@gmail.com"},
            {"password", "password123"}
        };

        json login_response = client.sendRequest(login_request);
        cout << "로그인 응답:\n" << login_response.dump(2) << endl << endl;

        // 방 생성 요청
        json create_room_request = {
            {"action", "createRoom"},
            {"roomName", "Test Room"},
            {"maxPlayers", 4}
        };

        json create_room_response = client.sendRequest(create_room_request);
        cout << "방 생성 응답:\n" << create_room_response.dump(2) << endl << endl;

        client.disconnect();
    }
    catch (const std::exception& e) {
        cerr << "오류 발생: " << e.what() << endl;
    }

    return 0;
}