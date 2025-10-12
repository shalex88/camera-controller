#pragma once

#include <string>
#include <vector>

#include "infrastructure/camera/transport/ITransport.h"
#include "common/types/Result.h"

namespace camera_service::infrastructure {
    class TcpClient : public ITransport {
    public:
        explicit TcpClient(const std::string& device_path);
        ~TcpClient() override;

        Result<void> open() override;
        Result<void> close() override;
        bool isOpen() const override;
        Result<size_t> write(std::span<const std::byte> data) override;
        Result<std::vector<std::byte>> read() override;

    private:
        std::string ip_;
        uint16_t port_ = 0;
        int socket_fd_ = -1;
        bool is_connected_ = false;
    };
}