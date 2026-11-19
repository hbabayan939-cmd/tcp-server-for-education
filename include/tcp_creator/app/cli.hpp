#pragma once
#include "tcp_creator/config/server_config.hpp"

namespace tcp_creator::app {

enum class CliResult { Run, Help };

CliResult parse_arguments(int argc, char* argv[], config::ServerConfig& config);
void print_usage(const char* program_name);

}
