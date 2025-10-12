#pragma once

#include <string>

#include "infrastructure/camera/transport/ITransport.h"

namespace camera_service::infrastructure {
    class Uart final : public ITransport {
    public:
        explicit Uart(std::string device_path);
        ~Uart() override;

        Result<void> open() override;
        Result<void> close() override;
        Result<size_t> write(std::span<const std::byte> data) override;
        Result<std::vector<std::byte>> read() override;
        bool isOpen() const override;
        Result<void> configure(int baud_rate = 115200, int data_bits = 8,
                               int stop_bits = 1, char parity = 'N') const;
    private:
        std::string device_path_;
        int fd_ = -1;
    };
}
