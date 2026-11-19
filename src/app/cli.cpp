#include "tcp_creator/app/cli.hpp"
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace tcp_creator::app {
namespace {

std::string next_value(int& index, int argc, char* argv[], const std::string& option) {
    if (index + 1 >= argc) {
        throw std::invalid_argument("Missing value for " + option);
    }
    return argv[++index];
}

std::size_t positive_size(const std::string& value, const std::string& option,
                          std::size_t maximum = std::numeric_limits<std::size_t>::max()) {
    std::size_t consumed = 0;
    const auto parsed = std::stoull(value, &consumed);
    if (consumed != value.size() || parsed == 0 || parsed > maximum) {
        throw std::invalid_argument(option + " must be a positive number");
    }
    return static_cast<std::size_t>(parsed);
}

}

CliResult parse_arguments(int argc, char* argv[], config::ServerConfig& config) {
    for (int i = 1; i < argc; ++i) {
        const std::string option = argv[i];
        if (option == "--help" || option == "-h") {
            return CliResult::Help;
        }
        if (option == "--host") {
            config.host = next_value(i, argc, argv, option);
        } else if (option == "--port") {
            const auto value = next_value(i, argc, argv, option);
            const auto port = positive_size(value, option, 65535);
            config.port = static_cast<std::uint16_t>(port);
        } else if (option == "--workers") {
            config.worker_count = positive_size(next_value(i, argc, argv, option), option, 256);
        } else if (option == "--queue") {
            config.queue_capacity = positive_size(next_value(i, argc, argv, option), option, 100000);
        } else if (option == "--idle-timeout") {
            config.idle_timeout_seconds = static_cast<unsigned int>(positive_size(
                next_value(i, argc, argv, option), option, 86400));
        } else if (option == "--max-bytes") {
            config.max_bytes_per_client = positive_size(
                next_value(i, argc, argv, option), option, 1024ULL * 1024ULL * 1024ULL);
        } else {
            throw std::invalid_argument("Unknown option: " + option);
        }
    }
    return CliResult::Run;
}

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name
              << " [--host ADDRESS] [--port PORT] [--workers N] [--queue N]\n"
              << "       [--idle-timeout SECONDS] [--max-bytes N]\n"
              << "Start a bounded TCP echo server (defaults: 127.0.0.1:5050).\n";
}

}
