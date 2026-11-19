#pragma once
#include "tcp_creator/config/server_config.hpp"

namespace tcp_creator::net {

class TcpServer {
public:
    explicit TcpServer(config::ServerConfig config);
    void run();

private:
    config::ServerConfig config_;
};

}
