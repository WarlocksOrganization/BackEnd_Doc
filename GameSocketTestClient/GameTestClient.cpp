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

            // 응답 받기 - 개선된 방식
            boost::asio::streambuf response_buffer;
            boost::system::error_code ec;

            // 비동기 읽기 준비
            bool read_completed = false;
            std::size_t bytes_transferred = 0;

            // 비동기 읽기 시작
            socket_.async_read_some(
                boost::asio::buffer(response_buffer.prepare(1024)),
                [&](const boost::system::error_code& error, std::size_t bytes) {
                    ec = error;
                    bytes_transferred = bytes;
                    response_buffer.commit(bytes);
                    read_completed = true;
                }
            );

            // 타임아웃을 위한 실행
            for (int i = 0; i < 50 && !read_completed; ++i) {
                io_context_.poll();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            if (!read_completed) {
                cerr << "응답 대기 시간 초과" << endl;
                return json{ {"status", "error"}, {"message", "응답 대기 시간 초과"} };
            }

            if (ec && ec != boost::asio::error::eof) {
                throw boost::system::system_error(ec);
            }

            // 응답 데이터 처리
            std::string response_str((std::istreambuf_iterator<char>(&response_buffer)),
                std::istreambuf_iterator<char>());

            if (response_str.empty()) {
                return json{ {"status", "error"}, {"message", "서버 응답 없음"} };
            }

            cout << "받은 응답: " << response_str << endl;
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

        // 핸드셰이크
        json handshake_request = {
            {"connectionType", "mirror"}
        };

        json handshake_response = client.sendRequest(handshake_request);
        cout << "핸드셰이크 응답:\n" << handshake_response.dump(2) << endl << endl;
        Sleep(1000);

        //// 회원가입 요청
        //json register_request = {
        //    {"action", "register"},
        //    {"userName", "APItestUser"},
        //    {"password", "testest"}
        //};

        //json register_response = client.sendRequest(register_request);
        //cout << "회원가입 응답:\n" << register_response.dump(2) << endl << endl;
        //Sleep(1000);

        // 로그인 요청
        json login_request = {
            {"action", "login"},
            {"userName", "zzzzz955@gmail.com"},
            {"password", "testest"}
        };

        json login_response = client.sendRequest(login_request);
        cout << "로그인 응답:\n" << login_response.dump(2) << endl << endl;
        Sleep(1000);

        json exit_room_request = {
            {"action", "exitRoom"}
        };
        json exit_room_response = client.sendRequest(exit_room_request);
        cout << "방 퇴장 응답:\n" << exit_room_response.dump(2) << endl << endl;

        //// 방 생성 요청
        //json create_room_request = {
        //    {"action", "createRoom"},
        //    {"roomName", "Test Room"},
        //    {"maxPlayers", 4}
        //};

        //json create_room_response = client.sendRequest(create_room_request);
        //cout << "방 생성 응답:\n" << create_room_response.dump(2) << endl << endl;

        client.disconnect();
    }
    catch (const std::exception& e) {
        cerr << "오류 발생: " << e.what() << endl;
    }

    return 0;
}