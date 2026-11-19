# TCP Server Creator

A modular C++17 TCP echo server. It is organized into application, configuration, networking, and protocol tasks so you can extend each part independently.

## Project layout

```text
include/tcp_creator/       Public headers by responsibility
  app/                     Command-line parsing
  config/                  Server configuration
  net/                     TCP listener and bounded client pool
  protocol/                Echo protocol
src/                       C++ implementations matching the header layout
CMakeLists.txt             CMake build configuration
Dockerfile                 Multi-stage container build
requirements.txt           No Python packages are required
```

## Build

Requires a C++17 compiler, CMake 3.16 or newer, and POSIX sockets (Linux/macOS).

```sh
cmake -S . -B build
cmake --build build --parallel
```

## Run

```sh
./build/tcp-server
./build/tcp-server --host 127.0.0.1 --port 9000
./build/tcp-server --help
```

The default bind address is `127.0.0.1:5050`, so the server is only reachable from the local machine. To accept remote connections, explicitly set `--host 0.0.0.0` and restrict access with a firewall.

The server uses a fixed worker pool (8 workers by default) and a bounded pending-connection queue (64 connections). When the queue is full, new clients are closed. Each client has a 30-second socket I/O timeout and a 1 MiB transfer limit; exceeding the byte limit closes the connection. Ctrl+C and SIGTERM stop accepting new connections and let active workers finish or time out.

Available options:

| Option | Default | Purpose |
| --- | --- | --- |
| `--host ADDRESS` | `127.0.0.1` | Address to bind |
| `--port PORT` | `5050` | Listening TCP port |
| `--workers N` | `8` | Worker threads (1–256) |
| `--queue N` | `64` | Maximum queued client connections |
| `--idle-timeout SECONDS` | `30` | Per-operation socket timeout (1–86400) |
| `--max-bytes N` | `1048576` | Maximum bytes echoed per connection |

Try it with netcat:

```sh
nc 127.0.0.1 5050
```

## Docker

The container binds to `0.0.0.0` inside the container; publish only the port you intend to expose:

```sh
docker build -t tcp-server .
docker run --rm -p 127.0.0.1:5050:5050 tcp-server
```

For remote access, configure the host firewall deliberately. To change the port, pass the option to the container and publish the matching port, for example `docker run --rm -p 127.0.0.1:9000:9000 tcp-server --host 0.0.0.0 --port 9000`.

## Extending

Implement a protocol handler in `include/tcp_creator/protocol/` and `src/protocol/`, then call it from the networking layer. Keep command-line parsing in `app/`, settings in `config/`, and socket lifecycle code in `net/`.

This is still an educational plain TCP echo server: it does not provide TLS or client authentication. Do not send secrets over it or expose it to untrusted networks without adding those protections.
# tcp-server-for-education
