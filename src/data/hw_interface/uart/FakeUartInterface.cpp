#include "FakeUartInterface.h"
#include "common/Logger/Logger.h"

namespace camera_service::data::uart {
    Result<void> FakeUartInterface::open() {
        if (is_open_) {
            return Result<void>::error("Fake UART device is already open");
        }

        is_open_ = true;
        stored_data_.clear();

        return Result<void>::success();
    }

    Result<void> FakeUartInterface::close() {
        if (!is_open_) {
            return Result<void>::success();
        }

        is_open_ = false;
        stored_data_.clear();

        return Result<void>::success();
    }

    Result<void> FakeUartInterface::configure(const int baud_rate, const int data_bits, const int stop_bits, const char parity) {
        if (!is_open_) {
            return Result<void>::error("Fake UART device is not open");
        }

        LOG_DEBUG("Fake UART configured: {}bps, {}{}{}",
                  baud_rate, data_bits, parity, stop_bits);
        return Result<void>::success();
    }

    Result<size_t> FakeUartInterface::write(const std::vector<char>& data) {
        stored_data_ = data;
        return Result<size_t>::success(data.size());
    }

    Result<std::vector<char>> FakeUartInterface::read() {
        return Result<std::vector<char>>::success(stored_data_);
    }

    bool FakeUartInterface::isOpen() const {
        return is_open_;
    }
}
