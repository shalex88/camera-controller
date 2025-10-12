#pragma once

#include <cstddef>
#include <span>
#include <string_view>

#include "common/types/Result.h"

namespace camera_service::infrastructure {
    class ITransport {
    public:
        virtual ~ITransport() = default;
        virtual Result<void> open() = 0; //TODO: do we need it in the interface or only use RAII?
        virtual Result<void> close() = 0; //TODO: do we need it in the interface or only use RAII?
        virtual Result<size_t> write(std::span<const std::byte> data) = 0;
        virtual Result<std::vector<std::byte>> read() = 0;
        virtual bool isOpen() const = 0;

        Result<size_t> write(std::string_view sv) {
            const std::span chars{sv.data(), sv.size()};
            const auto bytes = std::as_bytes(chars);
            return write(bytes);
        }
    };
}
