#pragma once

#include "IUartInterface.h"
#include <string>

namespace camera_service::data::uart {
    enum class DeviceType {
        UART,
        PTS
    };

    class UartInterface final : public IUartInterface {
    public:
        explicit UartInterface(std::string device_path);
        ~UartInterface() override;

        Result<void> open() override;
        Result<void> close() override;
        Result<void> configure(int baud_rate = 115200, int data_bits = 8,
                             int stop_bits = 1, char parity = 'N') override;
        Result<size_t> write(const std::vector<char>& data) override;
        Result<std::vector<char>> read() override;
        bool isOpen() const override;

    private:
        std::string device_path_;
        int fd_ = -1;
        DeviceType device_type_;

        DeviceType detectDeviceType() const;
        Result<void> setTerminalAttributes(int baud_rate, int data_bits, int stop_bits, char parity);
        static int getBaudRateFlag(int baud_rate);
    };
}
