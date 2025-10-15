#include "TcpClient.h"

#include <stdexcept>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

namespace camera_service::infrastructure {
    TcpClient::TcpClient(const std::string& device_path) {
        if (device_path.empty()) {
            throw std::invalid_argument("Device path cannot be empty");
        }

        const auto colon_pos = device_path.find(":");
        if (colon_pos == std::string::npos || colon_pos == 0 || colon_pos == device_path.size() - 1) {
            throw std::invalid_argument("Device path must be in format <ip>:<port>");
        }
        ip_ = device_path.substr(0, colon_pos);
        const std::string port_str = device_path.substr(colon_pos + 1);
        try {
            const int port_val = std::stoi(port_str);
            if (port_val < 1 || port_val > 65535) {
                throw std::out_of_range("Port out of range");
            }
            port_ = static_cast<uint16_t>(port_val);
        } catch (const std::exception& e) {
            throw std::invalid_argument(std::string("Invalid port in device path: ") + e.what());
        }

        if (open().isError()) {
            throw std::runtime_error("Failed to open TCP client transport");
        }
    }

    TcpClient::~TcpClient() {
        if (close().isError()) {
            LOG_ERROR("Failed to close TCP client transport");
        }
    }

    Result<void> TcpClient::open() {
        if (is_connected_) {
            return Result<void>::success();
        }
        socket_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
        if (socket_fd_ < 0) {
            return Result<void>::error("Failed to create socket");
        }
        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port_);
        if (::inet_pton(AF_INET, ip_.c_str(), &server_addr.sin_addr) <= 0) {
            ::close(socket_fd_);
            socket_fd_ = -1;
            return Result<void>::error("Invalid address");
        }
        if (::connect(socket_fd_, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
            ::close(socket_fd_);
            socket_fd_ = -1;
            return Result<void>::error("Connection failed");
        }
        is_connected_ = true;
        return Result<void>::success();
    }

    Result<void> TcpClient::close() {
        if (is_connected_ && socket_fd_ >= 0) {
            ::close(socket_fd_);
            socket_fd_ = -1;
            is_connected_ = false;
        }
        return Result<void>::success();
    }

    bool TcpClient::isOpen() const {
        if (!is_connected_ || socket_fd_ < 0) {
            return false;
        }
        return true;
    }

    Result<void> TcpClient::write(std::span<const std::byte> data) {
        if (!isOpen()) {
            return Result<void>::error("Not connected");
        }

        size_t total_sent = 0;

        while (total_sent < data.size()) {
            const ssize_t sent = ::send(socket_fd_, data.data() + total_sent, data.size() - total_sent, 0);
            if (sent < 0) {
                return Result<void>::error("Send failed");
            }
            total_sent += static_cast<size_t>(sent);
        }
        return Result<void>::success();
    }

    Result<std::vector<std::byte>> TcpClient::read() {
        if (!isOpen()) {
            return Result<std::vector<std::byte>>::error("Not connected");
        }

        constexpr size_t MAX_BYTES = 4096;
        constexpr timeval TIMEOUT_DEFAULT{1, 0};

        while (true) {
            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(socket_fd_, &read_fds);

            // reinitialize timeout each select call because select may modify it
            timeval timeout = TIMEOUT_DEFAULT;

            const auto select_result = ::select(socket_fd_ + 1, &read_fds, nullptr, nullptr, &timeout);
            if (select_result < 0) {
                if (errno == EINTR) {
                    // interrupted by signal, retry
                    continue;
                }
                return Result<std::vector<std::byte>>::error(std::string("Select failed: ") + std::strerror(errno));
            }
            if (select_result == 0) {
                return Result<std::vector<std::byte>>::error("Read timeout");
            }

            if (!FD_ISSET(socket_fd_, &read_fds)) {
                // unexpected: select reported activity but socket not set
                return Result<std::vector<std::byte>>::error("Select returned without socket readiness");
            }

            std::vector<std::byte> buffer(MAX_BYTES);
            const ssize_t bytes_read = ::recv(socket_fd_, buffer.data(), buffer.size(), 0);
            if (bytes_read < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // try again (non-blocking case)
                    continue;
                }
                return Result<std::vector<std::byte>>::error(
                    std::string("Failed to receive from TCP: ") + std::strerror(errno));
            }
            if (bytes_read == 0) {
                // peer performed orderly shutdown
                return Result<std::vector<std::byte>>::error("Connection closed by peer");
            }

            buffer.resize(static_cast<size_t>(bytes_read));
            return Result<std::vector<std::byte>>::success(std::move(buffer));
        }
    }
}