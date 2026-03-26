#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <span>
#include <vector>

#include "common/types/Result.h"

namespace service::infrastructure {
    class ITransport;

    class ItlProtocol {
    public:
        explicit ItlProtocol(std::unique_ptr<ITransport> transport);
        ~ItlProtocol();

        Result<void> open() const;
        Result<void> close() const;
        Result<std::vector<std::byte>> send(std::uint32_t device_id, std::uint32_t opcode) const;
        Result<std::vector<std::byte>> send(std::uint32_t device_id, std::uint32_t opcode, std::span<const std::byte> payload) const;

    private:
        std::unique_ptr<ITransport> transport_;
        mutable std::array<std::byte, 128> rx_buffer_{};
    };
} // namespace service::infrastructure
