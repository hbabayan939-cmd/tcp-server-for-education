#include "tcp_creator/protocol/echo_protocol.hpp"
#include <sys/socket.h>

#include <cerrno>
#include <stdexcept>
#include <algorithm>

namespace tcp_creator::protocol {

void EchoProtocol::handle(int client_fd, std::size_t max_bytes) {
    char buffer[4096];
    std::size_t total = 0;
    while (true) {
        const auto remaining = max_bytes - total;
        const auto receive_size = std::min(sizeof(buffer), remaining + 1);
        const auto received = recv(client_fd, buffer, receive_size, 0);
        if (received == 0) return;
        if (received < 0) {
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) return;
            throw std::runtime_error("recv failed");
        }
        if (static_cast<std::size_t>(received) > remaining) return;

        ssize_t sent_total = 0;
        while (sent_total < received) {
            const auto sent = send(client_fd, buffer + sent_total,
                                   static_cast<std::size_t>(received - sent_total), 0);
            if (sent < 0 && errno == EINTR) continue;
            if (sent <= 0) throw std::runtime_error("send failed");
            sent_total += sent;
        }
        total += static_cast<std::size_t>(received);
    }
}

}
