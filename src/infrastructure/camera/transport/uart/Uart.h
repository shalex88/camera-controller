#pragma once

#include <string>
#include <termios.h>

#include "infrastructure/camera/transport/ITransport.h"

namespace camera_service::infrastructure {
    enum class ResponseType : uint32_t {
        Clear = 0x40,
        Address = 0x30,
        Ack = 0x40,
        Completed = 0x50,
        Error = 0x60
    };

    struct ViscaInterface {
        // RS232 data:
        int port_fd;
        termios options;
        uint32_t baud;
        // VISCA data:
        uint32_t address;
        uint32_t broadcast;
        // RS232 input buffer
        uint8_t ibuf[1024];
        uint32_t size;
        ResponseType type;
    };

    class Uart final : public ITransport {
    public:
        explicit Uart(std::string device_path);
        ~Uart() override;

        Result<void> open() override;
        Result<void> close() override;
        Result<void> write(std::span<const std::byte> data) override;
        Result<std::vector<std::byte>> read() override;
        bool isOpen() const override;
        Result<void> configure(int baud_rate = 115200, int data_bits = 8,
                               int stop_bits = 1, char parity = 'N') const;
        ViscaInterface iface{};

    private:
        std::string device_path_;
    };
}
