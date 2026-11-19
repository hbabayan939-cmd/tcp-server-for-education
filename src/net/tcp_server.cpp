#include "tcp_creator/net/tcp_server.hpp"
#include "tcp_creator/protocol/echo_protocol.hpp"

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <cerrno>
#include <csignal>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

namespace tcp_creator::net {
namespace {

volatile std::sig_atomic_t running = 1;

void handle_signal(int) { running = 0; }

class SocketAddressList {
public:
    explicit SocketAddressList(addrinfo* value) : value_(value) {}
    ~SocketAddressList() { freeaddrinfo(value_); }
    SocketAddressList(const SocketAddressList&) = delete;
    SocketAddressList& operator=(const SocketAddressList&) = delete;
    addrinfo* get() const { return value_; }

private:
    addrinfo* value_;
};

int create_listener(const config::ServerConfig& config) {
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    addrinfo* raw_addresses = nullptr;
    const auto service = std::to_string(config.port);
    const char* host = config.host == "0.0.0.0" ? nullptr : config.host.c_str();
    const int result = getaddrinfo(host, service.c_str(), &hints, &raw_addresses);
    if (result != 0) {
        throw std::runtime_error(std::string("Address lookup failed: ") + gai_strerror(result));
    }
    SocketAddressList addresses(raw_addresses);

    for (auto* address = addresses.get(); address != nullptr; address = address->ai_next) {
        const int fd = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
        if (fd < 0) continue;

        int reuse = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
        if (bind(fd, address->ai_addr, address->ai_addrlen) == 0 &&
            listen(fd, config.backlog) == 0) {
            return fd;
        }
        close(fd);
    }
    throw std::runtime_error("Could not bind a listening socket to " + config.host + ":" + service);
}

void configure_client_socket(int client_fd, unsigned int idle_timeout_seconds) {
    const timeval timeout{static_cast<time_t>(idle_timeout_seconds), 0};
    if (setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0 ||
        setsockopt(client_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
        throw std::runtime_error("Could not configure client socket timeouts");
    }
}

class ClientPool {
public:
    explicit ClientPool(const config::ServerConfig& config)
        : queue_capacity_(config.queue_capacity),
          idle_timeout_seconds_(config.idle_timeout_seconds),
          max_connection_seconds_(config.max_connection_seconds),
          max_bytes_per_client_(config.max_bytes_per_client) {
        try {
            for (std::size_t i = 0; i < config.worker_count; ++i) {
                workers_.emplace_back([this] { worker_loop(); });
            }
        } catch (...) {
            stop();
            throw;
        }
    }

    ~ClientPool() { stop(); }
    ClientPool(const ClientPool&) = delete;
    ClientPool& operator=(const ClientPool&) = delete;

    bool enqueue(int client_fd) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (stopping_ || queue_.size() >= queue_capacity_) return false;
            queue_.push_back(client_fd);
        }
        condition_.notify_one();
        return true;
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (stopping_) return;
            stopping_ = true;
            for (const int client_fd : queue_) close(client_fd);
            queue_.clear();
        }
        condition_.notify_all();
        for (auto& worker : workers_) {
            if (worker.joinable()) worker.join();
        }
    }

private:
    void worker_loop() {
        while (true) {
            int client_fd = -1;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                condition_.wait(lock, [this] { return stopping_ || !queue_.empty(); });
                if (stopping_ && queue_.empty()) return;
                client_fd = queue_.front();
                queue_.pop_front();
            }
            try {
                configure_client_socket(client_fd, idle_timeout_seconds_);
                protocol::EchoProtocol::handle(client_fd, max_bytes_per_client_,
                                               idle_timeout_seconds_, max_connection_seconds_);
            } catch (const std::exception& error) {
                std::cerr << "Client error: " << error.what() << '\n';
            }
            close(client_fd);
        }
    }

    const std::size_t queue_capacity_;
    const unsigned int idle_timeout_seconds_;
    const unsigned int max_connection_seconds_;
    const std::size_t max_bytes_per_client_;
    std::mutex mutex_;
    std::condition_variable condition_;
    std::deque<int> queue_;
    std::vector<std::thread> workers_;
    bool stopping_ = false;
};

}

TcpServer::TcpServer(config::ServerConfig config) : config_(std::move(config)) {}

void TcpServer::run() {
    running = 1;
    struct sigaction action{};
    action.sa_handler = handle_signal;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGINT, &action, nullptr);
    sigaction(SIGTERM, &action, nullptr);
    struct sigaction pipe_action{};
    pipe_action.sa_handler = SIG_IGN;
    sigemptyset(&pipe_action.sa_mask);
    sigaction(SIGPIPE, &pipe_action, nullptr);

    ClientPool clients(config_);
    const int listener = create_listener(config_);
    std::cout << "Listening on " << config_.host << ':' << config_.port
              << " with " << config_.worker_count << " workers (press Ctrl+C to stop)\n";

    while (running != 0) {
        const int client_fd = accept(listener, nullptr, nullptr);
        if (client_fd < 0) {
            if (errno == EINTR && running == 0) break;
            if (errno == EINTR) continue;
            if (running == 0) break;
            std::cerr << "accept failed: " << std::strerror(errno) << '\n';
            continue;
        }

        if (!clients.enqueue(client_fd)) close(client_fd);
    }
    close(listener);
    clients.stop();
    std::cout << "Server stopped.\n";
}

}
