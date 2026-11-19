#pragma once
#include <cstddef>
#include <cstdint>

namespace tcp_creator::protocol {

class EchoProtocol {
public:
    static void handle(int client_fd, std::size_t max_bytes,
                       std::uint32_t idle_timeout_seconds,
                       std::uint32_t max_connection_seconds);
};

}
