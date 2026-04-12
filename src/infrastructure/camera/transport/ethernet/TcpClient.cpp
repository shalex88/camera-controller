#include "TcpClient.h"

#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <memory>
#include <stdexcept>
#include <unistd.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/socket.h>

namespace service::infrastructure {
    TcpClient::TcpClient(const std::string& device_path) {
        if (device_path.empty()) {
            throw std::invalid_argument("Device path cannot be empty");
        }

        const auto colon_pos = device_path.find(':');
        if (colon_pos == std::string::npos || colon_pos == 0 || colon_pos == device_path.size() - 1) {
            throw std::invalid_argument("Device path must be in format <host>:<port>");
        }
        host_ = device_path.substr(0, colon_pos);
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

        addrinfo hints{};
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        addrinfo* raw_addr_list = nullptr;
        const std::string port_str = std::to_string(port_);
        const int gai_result = ::getaddrinfo(host_.c_str(), port_str.c_str(), &hints, &raw_addr_list);
        if (gai_result != 0) {
            return Result<void>::error(
                std::string("Failed to resolve host '") + host_ + "': " + ::gai_strerror(gai_result));
        }
        auto addr_list = std::unique_ptr<addrinfo, decltype(&::freeaddrinfo)>(raw_addr_list, ::freeaddrinfo);

        for (addrinfo* addr = addr_list.get(); addr != nullptr; addr = addr->ai_next) {
            socket_fd_ = ::socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
            if (socket_fd_ < 0) {
                continue;
            }

            int flags = ::fcntl(socket_fd_, F_GETFL, 0);
            if (flags < 0 || ::fcntl(socket_fd_, F_SETFL, flags | O_NONBLOCK) < 0) {
                ::close(socket_fd_);
                socket_fd_ = -1;
                continue;
            }

            const int connect_result = ::connect(socket_fd_, addr->ai_addr, addr->ai_addrlen);
            bool connected = (connect_result == 0);

            if (!connected && errno == EINPROGRESS) {
                fd_set write_fds;
                FD_ZERO(&write_fds);
                FD_SET(socket_fd_, &write_fds);

                timeval timeout{3, 0};
                const int select_result = ::select(socket_fd_ + 1, nullptr, &write_fds, nullptr, &timeout);

                if (select_result > 0) {
                    int error = 0;
                    socklen_t len = sizeof(error);
                    if (::getsockopt(socket_fd_, SOL_SOCKET, SO_ERROR, &error, &len) == 0 && error == 0) {
                        connected = true;
                    }
                }
            }

            if (connected) {
                if (::fcntl(socket_fd_, F_SETFL, flags) < 0) {
                    ::close(socket_fd_);
                    socket_fd_ = -1;
                    continue;
                }
                is_connected_ = true;
                return Result<void>::success();
            }

            ::close(socket_fd_);
            socket_fd_ = -1;
        }

        return Result<void>::error("Failed to connect to " + host_ + ":" + port_str);
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

    Result<void> TcpClient::write(const std::span<const std::byte> tx_data) {
        if (!isOpen()) {
            return Result<void>::error("Not connected");
        }

        size_t total_sent = 0;

        while (total_sent < tx_data.size()) {
            const ssize_t sent = ::send(socket_fd_, tx_data.data() + total_sent, tx_data.size() - total_sent, 0);
            if (sent < 0) {
                return Result<void>::error("Write failed");
            }
            total_sent += static_cast<size_t>(sent);
        }
        return Result<void>::success();
    }

    Result<size_t> TcpClient::read(std::span<std::byte> rx_data) {
        if (!isOpen()) {
            return Result<size_t>::error("Not connected");
        }

        while (true) {
            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(socket_fd_, &read_fds);

            // reinitialize timeout each select call because select may modify it
            timeval timeout{1, 0};

            const auto select_result = ::select(socket_fd_ + 1, &read_fds, nullptr, nullptr, &timeout);
            if (select_result < 0) {
                if (errno == EINTR) {
                    // interrupted by signal, retry
                    continue;
                }
                return Result<size_t>::error(std::string("Select failed: ") + std::strerror(errno));
            }
            if (select_result == 0) {
                return Result<size_t>::error("Read failed");
            }

            if (!FD_ISSET(socket_fd_, &read_fds)) {
                // unexpected: select reported activity but socket not set
                return Result<size_t>::error("Select returned without socket readiness");
            }

            const ssize_t bytes_read = ::recv(socket_fd_, rx_data.data(), rx_data.size(), 0);
            if (bytes_read < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // try again (non-blocking case)
                    continue;
                }
                return Result<size_t>::error(
                    std::string("Failed to receive from TCP: ") + std::strerror(errno));
            }
            if (bytes_read == 0) {
                // peer performed orderly shutdown
                return Result<size_t>::error("Connection closed by peer");
            }

            return Result<void>::success(static_cast<size_t>(bytes_read));
        }
    }
} // namespace service::infrastructure