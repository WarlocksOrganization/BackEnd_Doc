#pragma once
#include <string>
#include <optional>

namespace game_server {

    struct Rooms {
        int room_id;
        std::string room_name;
        int host_id;
        std::string status;
    };

} // namespace game_server