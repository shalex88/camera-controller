#pragma once

#include <string>
#include <vector>

#include "common/types/Result.h"

namespace camera_service::infrastructure {
    class TcpClientTransport {
    public:
        explicit TcpClientTransport(const std::string& device_path);
        ~TcpClientTransport();

        Result<void> connect();
        Result<void> disconnect();
        Result<void> send(const std::vector<uint8_t>& data) const;
        Result<std::vector<uint8_t>> receive() const;

    private:
        std::string ip_;
        uint16_t port_ = 0;
        int socket_fd_ = -1;
        bool is_connected_ = false;
    };
}