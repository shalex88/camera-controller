#pragma once

#include <shared_mutex>
#include <vector>

#include "infrastructure/camera/transport/ITransport.h"

namespace camera_service::infrastructure::uart {
    class FakeUart final : public ITransport {
    public:
        FakeUart();
        ~FakeUart() override;

        Result<void> open() override;
        Result<void> close() override;
        Result<size_t> write(std::span<const std::byte> data) override;
        Result<std::vector<std::byte>> read() override;
        bool isOpen() const override;
        Result<void> configure(int baud_rate = 115200, int data_bits = 8,
                             int stop_bits = 1, char parity = 'N') const;
    private:
        bool is_open_ = false;
        std::vector<std::byte> stored_data_;
        mutable std::shared_mutex data_mutex_;
    };
}
