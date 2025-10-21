#pragma once

#include <string>
#include <termios.h>

#include "infrastructure/camera/transport/ITransport.h"

namespace camera_service::infrastructure {
    class Uart final : public ITransport {
    public:
        explicit Uart(std::string device_path);
        ~Uart() override;

        Result<void> open() override;
        Result<void> close() override;
        Result<void> write(std::span<const std::byte> data) override;
        Result<void> read(std::span<std::byte> rx_data) override;
        bool isOpen() const override;
        Result<void> configure(int baud_rate = 115200, int data_bits = 8,
                               int stop_bits = 1, char parity = 'N') const;

    private:
        std::string device_path_;
        int port_fd_ = -1;
        termios options_{};
        uint32_t baud_ = 0;
    };
}
