#include "tcp_creator/protocol/echo_protocol.hpp"

#include <poll.h>
#include <sys/socket.h>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace tcp_creator::protocol {
namespace {

using Clock = std::chrono::steady_clock;

bool wait_for_io(int client_fd, short events, Clock::time_point deadline) {
    while (true) {
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline - Clock::now()).count();
        if (remaining <= 0) return false;

        pollfd descriptor{client_fd, events, 0};
        const int timeout_ms = static_cast<int>(std::min<std::int64_t>(
            remaining, std::numeric_limits<int>::max()));
        const int result = poll(&descriptor, 1, timeout_ms);
        if (result > 0) return true;
        if (result == 0) return false;
        if (errno != EINTR) throw std::runtime_error("poll failed");
    }
}

}  // namespace

void EchoProtocol::handle(int client_fd, std::size_t max_bytes,
                          std::uint32_t idle_timeout_seconds,
                          std::uint32_t max_connection_seconds) {
    char buffer[4096];
    std::size_t total = 0;
    const auto deadline = Clock::now() + std::chrono::seconds(max_connection_seconds);
    while (true) {
        const auto remaining = max_bytes - total;
        const auto receive_size = std::min(sizeof(buffer), remaining + 1);
        const auto read_deadline = std::min(
            deadline, Clock::now() + std::chrono::seconds(idle_timeout_seconds));
        if (!wait_for_io(client_fd, POLLIN, read_deadline)) return;

        const auto received = recv(client_fd, buffer, receive_size, 0);
        if (received == 0) return;
        if (received < 0) {
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) return;
            throw std::runtime_error("recv failed");
        }
        if (static_cast<std::size_t>(received) > remaining) return;

        ssize_t sent_total = 0;
        const auto write_deadline = std::min(
            deadline, Clock::now() + std::chrono::seconds(idle_timeout_seconds));
        while (sent_total < received) {
            if (!wait_for_io(client_fd, POLLOUT, write_deadline)) return;
            const auto sent = send(client_fd, buffer + sent_total,
                                   static_cast<std::size_t>(received - sent_total), 0);
            if (sent < 0 && errno == EINTR) continue;
            if (sent < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return;
            if (sent <= 0) throw std::runtime_error("send failed");
            sent_total += sent;
        }
        total += static_cast<std::size_t>(received);
    }
}

}  // namespace tcp_creator::protocol
