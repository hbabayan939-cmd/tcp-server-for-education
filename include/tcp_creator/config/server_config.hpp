#pragma once
#include <cstdint>
#include <cstddef>
#include <string>

namespace tcp_creator::config {

struct ServerConfig {
    std::string host = "127.0.0.1";
    std::uint16_t port = 5050;
    int backlog = 64;
    std::size_t worker_count = 8;
    std::size_t queue_capacity = 64;
    unsigned int idle_timeout_seconds = 30;
    unsigned int max_connection_seconds = 120;
    std::size_t max_bytes_per_client = 1024 * 1024;
};

}
