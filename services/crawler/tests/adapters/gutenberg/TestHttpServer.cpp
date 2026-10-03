#include "crawler/tests/adapters/gutenberg/TestHttpServer.hpp"

#include <cstdint>
#include <cstring>
#include <stdexcept>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
using SocketLength = int;
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
using SocketLength = socklen_t;
#endif

namespace tarantino::crawler::adapters {

namespace {

#ifdef _WIN32
using Socket = SOCKET;
constexpr Socket NO_SOCKET = INVALID_SOCKET;

void closeSocket(Socket socket) {
    closesocket(socket);
}

void startSockets() {
    static const bool started = [] {
        WSADATA data;
        return WSAStartup(MAKEWORD(2, 2), &data) == 0;
    }();
    if (!started) {
        throw std::runtime_error("Winsock could not start");
    }
}
#else
using Socket = int;
constexpr Socket NO_SOCKET = -1;

void closeSocket(Socket socket) {
    ::close(socket);
}

void startSockets() {
}
#endif

#ifdef MSG_NOSIGNAL
constexpr int SEND_FLAGS = MSG_NOSIGNAL;
#else
constexpr int SEND_FLAGS = 0;
#endif

std::string reason(int status) {
    switch (status) {
        case 200:
            return "OK";
        case 301:
            return "Moved Permanently";
        case 302:
            return "Found";
        case 404:
            return "Not Found";
        case 429:
            return "Too Many Requests";
        case 503:
            return "Service Unavailable";
        default:
            return "Status";
    }
}

}

TestHttpServer::TestHttpServer(Handler handler) : handler_(std::move(handler)) {
    startSockets();
    const Socket listening = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listening == NO_SOCKET) {
        throw std::runtime_error("Could not open a socket");
    }
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = 0;
    SocketLength length = sizeof address;
    if (::bind(listening, reinterpret_cast<sockaddr*>(&address), sizeof address) != 0
        || ::listen(listening, 16) != 0
        || ::getsockname(listening, reinterpret_cast<sockaddr*>(&address), &length) != 0) {
        closeSocket(listening);
        throw std::runtime_error("Could not listen on 127.0.0.1");
    }
    socket_ = static_cast<std::intptr_t>(listening);
    port_ = ntohs(address.sin_port);
    thread_ = std::thread([this] { serve(); });
}

TestHttpServer::~TestHttpServer() {
    stopping_ = true;
    thread_.join();
    closeSocket(static_cast<Socket>(socket_));
}

void TestHttpServer::serve() {
    const auto listening = static_cast<Socket>(socket_);
    while (!stopping_) {
        fd_set readable;
        FD_ZERO(&readable);
        FD_SET(listening, &readable);
        timeval timeout{0, 20'000};
        if (::select(static_cast<int>(listening) + 1, &readable, nullptr, nullptr, &timeout) <= 0) {
            continue;
        }
        const Socket connection = ::accept(listening, nullptr, nullptr);
        if (connection != NO_SOCKET) {
#ifdef SO_NOSIGPIPE
            // macOS has no MSG_NOSIGNAL: a client that hangs up must not stop the tests with SIGPIPE
            const int on = 1;
            ::setsockopt(connection, SOL_SOCKET, SO_NOSIGPIPE, &on, sizeof on);
#endif
            answer(static_cast<std::intptr_t>(connection));
            closeSocket(connection);
        }
    }
}

void TestHttpServer::answer(std::intptr_t socket) {
    const auto connection = static_cast<Socket>(socket);
    std::string request;
    char buffer[4096];
    while (request.find("\r\n\r\n") == std::string::npos) {
        const int received = static_cast<int>(::recv(connection, buffer, sizeof buffer, 0));
        if (received <= 0) {
            return;
        }
        request.append(buffer, static_cast<std::size_t>(received));
    }
    // GET <path> HTTP/1.1
    const std::size_t pathStart = request.find(' ') + 1;
    const std::string path = request.substr(pathStart, request.find(' ', pathStart) - pathStart);
    const Answer reply = handler_(path);
    std::string response = "HTTP/1.1 " + std::to_string(reply.status) + " " + reason(reply.status) + "\r\n";
    for (const auto& [name, value] : reply.headers) {
        response += name + ": " + value + "\r\n";
    }
    response += "Content-Length: " + std::to_string(reply.body.size()) + "\r\nConnection: close\r\n\r\n" + reply.body;
    std::size_t sent = 0;
    while (sent < response.size()) {
        const int written = static_cast<int>(::send(connection, response.data() + sent, static_cast<int>(response.size() - sent), SEND_FLAGS));
        if (written <= 0) {
            return;
        }
        sent += static_cast<std::size_t>(written);
    }
}

}
