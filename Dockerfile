FROM debian:bookworm-slim AS build
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY . .
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build --parallel

FROM debian:bookworm-slim
WORKDIR /app
COPY --from=build /app/build/tcp-server /usr/local/bin/tcp-server
EXPOSE 5050
ENTRYPOINT ["tcp-server"]
CMD ["--host", "0.0.0.0"]
