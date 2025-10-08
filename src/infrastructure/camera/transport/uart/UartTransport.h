#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <termios.h>

constexpr uint32_t VISCA_INPUT_BUFFER_SIZE = 1024;

namespace camera_service::infrastructure {
    class UartTransport final {
    public:
        explicit UartTransport(const std::string& device_path);
        ~UartTransport();

        bool connect();
        void disconnect();

        bool read();
        bool write(std::span<uint8_t> buffer) const;

        // RS232 data:
        int port_fd_{-1};
        termios options_{};
        uint32_t baud_{9600};
        std::string device_path_{};
        // VISCA data:
        uint32_t address_{};
        uint32_t broadcast_{};
        // RS232 input buffer
        std::array<uint8_t, VISCA_INPUT_BUFFER_SIZE> ibuf_{};
        uint32_t bytes_{};
    };
}

