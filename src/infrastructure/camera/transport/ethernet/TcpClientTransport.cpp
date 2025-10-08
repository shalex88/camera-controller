#include "TcpClientTransport.h"

#include <stdexcept>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

namespace camera_service::infrastructure {
    TcpClientTransport::TcpClientTransport(const std::string& device_path) {
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
    }

    TcpClientTransport::~TcpClientTransport() {
        if (is_connected_) {
            if (disconnect().isError()) {
                // TODO: use layer logger
            }
        }
    }

    Result<void> TcpClientTransport::connect() {
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

    Result<void> TcpClientTransport::disconnect() {
        if (is_connected_ && socket_fd_ >= 0) {
            ::close(socket_fd_);
            socket_fd_ = -1;
            is_connected_ = false;
        }
        return Result<void>::success();
    }

    Result<void> TcpClientTransport::send(const std::vector<uint8_t>& data) const {
        if (!is_connected_ || socket_fd_ < 0) {
            return Result<void>::error("Not connected");
        }

        const size_t bytes_to_send = data.size();
        const auto* payload = reinterpret_cast<const char*>(data.data());
        size_t total_sent = 0;

        while (total_sent < bytes_to_send) {
            const ssize_t sent = ::send(socket_fd_, payload + total_sent, bytes_to_send - total_sent, 0);
            if (sent < 0) {
                return Result<void>::error("Send failed");
            }
            total_sent += static_cast<size_t>(sent);
        }
        return Result<void>::success();
    }

    Result<std::vector<uint8_t>> TcpClientTransport::receive() const {
        if (!is_connected_ || socket_fd_ < 0) {
            return Result<std::vector<uint8_t>>::error("Not connected");
        }
        constexpr size_t k_buffer_size = 4096;
        std::vector<uint8_t> response;
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(socket_fd_, &read_fds);
        timeval timeout {1, 0}; // 1 second timeout
        if (const int ready = select(socket_fd_ + 1, &read_fds, nullptr, nullptr, &timeout); ready < 0) {
            return Result<std::vector<uint8_t>>::error("Select failed");
        } else if (ready == 0) {
            // Timeout, no data
            return Result<std::vector<uint8_t>>::error("No answer");
        }
        // Data available
        uint8_t buffer[k_buffer_size] {};
        const ssize_t received = ::recv(socket_fd_, buffer, k_buffer_size, 0);
        if (received < 0) {
            return Result<std::vector<uint8_t>>::error("Receive failed");
        }
        response.insert(response.end(), buffer, buffer + received);

        return Result<std::vector<uint8_t>>::success(response);
    }
}
