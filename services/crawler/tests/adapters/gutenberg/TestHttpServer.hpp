#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace tarantino::crawler::adapters {

/// A minimal HTTP/1.1 server on 127.0.0.1, as the JDK's HttpServer in the Java tests: it answers each request with what
/// its handler returns for the request's path, one connection at a time.
class TestHttpServer {
public:
    struct Answer {
        int status = 200;
        std::vector<std::pair<std::string, std::string>> headers;
        std::string body;
    };

    using Handler = std::function<Answer(const std::string& path)>;

    explicit TestHttpServer(Handler handler);

    TestHttpServer(const TestHttpServer&) = delete;
    TestHttpServer& operator=(const TestHttpServer&) = delete;

    ~TestHttpServer();

    int port() const {
        return port_;
    }

private:
    void serve();

    void answer(std::intptr_t connection);

    Handler handler_;
    std::intptr_t socket_ = -1;
    int port_ = 0;
    std::atomic<bool> stopping_{false};
    std::thread thread_;
};

}
