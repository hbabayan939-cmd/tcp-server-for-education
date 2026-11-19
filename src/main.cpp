#include "tcp_creator/app/cli.hpp"
#include "tcp_creator/net/tcp_server.hpp"

#include <exception>
#include <iostream>
#include <utility>

int main(int argc, char* argv[]) {
    try {
        tcp_creator::config::ServerConfig config;
        if (tcp_creator::app::parse_arguments(argc, argv, config) ==
            tcp_creator::app::CliResult::Help) {
            tcp_creator::app::print_usage(argv[0]);
            return 0;
        }
        tcp_creator::net::TcpServer server(std::move(config));
        server.run();
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
