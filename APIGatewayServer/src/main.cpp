#include "api_gateway.h"
#include <boost/asio.hpp>
#include <spdlog/spdlog.h>

int main() {
    try {
        boost::asio::io_context io_context;
        ApiGateway gateway(io_context, 8080);

        spdlog::info("API Gateway started on port 8080");

        io_context.run();
    }
    catch (const std::exception& e) {
        spdlog::error("Gateway error: {}", e.what());
        return 1;
    }
    return 0;
}