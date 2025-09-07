#pragma once

#include "IUartInterface.h"
#include <string>
#include <map>
#include <vector>

namespace camera_service::data::uart {
    class FakeUartInterface final : public IUartInterface {
    public:
        FakeUartInterface() = default;
        ~FakeUartInterface() override = default;

        Result<void> open() override;
        Result<void> close() override;
        Result<void> configure(int baud_rate = 115200, int data_bits = 8,
                             int stop_bits = 1, char parity = 'N') override;
        Result<size_t> write(const std::vector<char>& data) override;
        Result<std::vector<char>> read() override;
        bool isOpen() const override;

    private:
        bool is_open_ = false;
        std::vector<char> stored_data_;
    };
}
