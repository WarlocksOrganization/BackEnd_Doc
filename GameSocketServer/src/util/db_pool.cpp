#include "db_pool.h"

// 표준 라이브러리 헤더 포함
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <stdexcept>

// 로깅 라이브러리
#include <spdlog/spdlog.h>

// PostgreSQL 클라이언트 라이브러리
#include <pqxx/pqxx>

namespace game_server {

    DbPool::DbPool(const std::string& connection_string, int pool_size)
        : connection_string_(connection_string)
    {
        try {
            // 연결 풀 초기화
            connections_.reserve(pool_size);
            in_use_.reserve(pool_size);

            for (int i = 0; i < pool_size; ++i) {
                // 새 연결 생성 및 풀에 추가
                auto conn = std::make_shared<pqxx::connection>(connection_string);
                connections_.push_back(conn);
                in_use_.push_back(false);

                spdlog::info("Created database connection {}/{}", i + 1, pool_size);
            }

            spdlog::info("Database connection pool initialized with {} connections", pool_size);
        }
        catch (const std::exception& e) {
            spdlog::error("Failed to initialize database connection pool: {}", e.what());
            throw;
        }
    }

    DbPool::~DbPool()
    {
        // 모든 연결 종료
        connections_.clear();
        spdlog::info("Database connection pool destroyed");
    }

    std::shared_ptr<pqxx::connection> DbPool::get_connection()
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // 사용 가능한 연결 찾기
        for (size_t i = 0; i < connections_.size(); ++i) {
            if (!in_use_[i]) {
                in_use_[i] = true;

                // 연결이 유효한지 확인
                if (!connections_[i]->is_open()) {
                    spdlog::warn("Connection {} was closed, reconnecting...", i);
                    try {
                        connections_[i] = std::make_shared<pqxx::connection>(connection_string_);
                    }
                    catch (const std::exception& e) {
                        spdlog::error("Failed to reconnect: {}", e.what());
                        in_use_[i] = false;
                        throw;
                    }
                }

                return connections_[i];
            }
        }

        // 사용 가능한 연결이 없으면 새 연결 생성
        spdlog::warn("No available connections in pool, creating a new one");
        try {
            auto conn = std::make_shared<pqxx::connection>(connection_string_);
            connections_.push_back(conn);
            in_use_.push_back(true);
            return conn;
        }
        catch (const std::exception& e) {
            spdlog::error("Failed to create new connection: {}", e.what());
            throw;
        }
    }

    void DbPool::return_connection(std::shared_ptr<pqxx::connection> conn)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // 반환된 연결 찾아서 사용 가능 상태로 변경
        for (size_t i = 0; i < connections_.size(); ++i) {
            if (connections_[i] == conn) {
                in_use_[i] = false;
                return;
            }
        }

        spdlog::warn("Attempted to return a connection not from this pool");
    }

} // namespace game_server