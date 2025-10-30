#pragma once

#include "common/types/Result.h"

namespace camera_service::infrastructure {
    struct PayloadObject {

    };

    class GenicamProtocol final {
        GenicamProtocol();
        ~GenicamProtocol();
        Result<void> open() const;
        Result<void> close() const;

        // Receives object fields, returns payload object
        Result<PayloadObject> pack(int field);
        // Receives payload object, returns payload raw bytes
        std::span<uint8_t> serialize(PayloadObject);
        // Receives payload, return frame
        std::vector<std::byte> encode(std::span<const std::byte> payload) const;
        // Receives frame
        Result<void> send(std::span<const std::byte> frame);

        // Receives buffer
        Result<size_t> receive(std::span<std::byte> buffer);
        // Receives buffer, return payload
        std::vector<std::byte> decode(std::span<const std::byte> buffer);
        // Receives payload raw bytes, returns payload object
        Result<PayloadObject> deserialize(std::span<const std::byte> data);
        // Receives payload object, returns object fields
        Result<int> unpack(PayloadObject);
    };
}
