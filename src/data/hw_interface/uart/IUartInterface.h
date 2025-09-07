#pragma once

#include <vector>

#include "common/types/Result.h"

namespace camera_service::data::uart {
    class IUartInterface {
    public:
        virtual ~IUartInterface() = default;

        virtual Result<void> open() = 0;
        virtual Result<void> close() = 0;
        virtual Result<void> configure(int baud_rate = 115200, int data_bits = 8,
                                     int stop_bits = 1, char parity = 'N') = 0;
        virtual Result<size_t> write(const std::vector<char>& data) = 0;
        virtual Result<std::vector<char>> read() = 0;
        virtual bool isOpen() const = 0;
    };
}
