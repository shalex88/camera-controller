#include "FakeUart.h"

#include "common/Logger/Logger.h"

namespace camera_service::infrastructure::uart {
    FakeUart::FakeUart() {
        if (open().isError()) {
            LOG_ERROR("Failed to open Fake UART interface");
        }
    }

    FakeUart::~FakeUart() {
        if (close().isError()) {
            LOG_ERROR("Failed to close Fake UART interface");
        }
    }

    Result<void> FakeUart::open() {
        std::unique_lock lock(data_mutex_);
        if (is_open_) {
            return Result<void>::error("Fake UART device is already open");
        }

        is_open_ = true;
        stored_data_.clear();

        return Result<void>::success();
    }

    Result<void> FakeUart::close() {
        std::unique_lock lock(data_mutex_);
        if (!is_open_) {
            return Result<void>::success();
        }

        is_open_ = false;
        stored_data_.clear();

        return Result<void>::success();
    }

    Result<void> FakeUart::configure(const int baud_rate, const int data_bits, const int stop_bits, const char parity) const {
        std::shared_lock lock(data_mutex_);
        if (!is_open_) {
            return Result<void>::error("Fake UART device is not open");
        }

        LOG_DEBUG("Fake UART configured: {}bps, {}{}{}",
                  baud_rate, data_bits, parity, stop_bits);
        return Result<void>::success();
    }

    Result<size_t> FakeUart::write(std::span<const std::byte> data) {
        std::unique_lock lock(data_mutex_);
        if (!is_open_) {
            return Result<size_t>::error("Fake UART device is not open");
        }

        stored_data_.assign(data.begin(), data.end());
        return Result<size_t>::success(data.size());
    }

    Result<std::vector<std::byte>> FakeUart::read() {
        std::shared_lock lock(data_mutex_);
        if (!is_open_) {
            return Result<std::vector<std::byte>>::error("Fake UART device is not open");
        }

        return Result<std::span<std::byte>>::success(stored_data_);
    }

    bool FakeUart::isOpen() const {
        std::shared_lock lock(data_mutex_);
        return is_open_;
    }
}
