cmake_minimum_required(VERSION 3.16)
project(tcp_server_creator VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

add_executable(tcp-server
    src/main.cpp
    src/app/cli.cpp
    src/net/tcp_server.cpp
    src/protocol/echo_protocol.cpp
)

target_include_directories(tcp-server PRIVATE include)
target_link_libraries(tcp-server PRIVATE Threads::Threads)

if(MSVC)
    target_compile_options(tcp-server PRIVATE /W4)
else()
    target_compile_options(tcp-server PRIVATE -Wall -Wextra -Wpedantic)
endif()

