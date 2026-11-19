#pragma once
#include <cstddef>

namespace tcp_creator::protocol {

class EchoProtocol {
public:
    static void handle(int client_fd, std::size_t max_bytes);
};

}
