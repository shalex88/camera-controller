#pragma once

#include <vector>
#include <memory>

#include "common/types/Result.h"
#include "TcpClientTransport.h"

namespace camera_service::infrastructure {
    struct __attribute__((packed)) ItlHeader {
        uint32_t opcode = 0;
        uint8_t id[4] = {'F', 'R', 'T', 'R'};
        uint16_t length = 0;
        uint16_t counter = 0;
        uint32_t time_stamp = 0;
        uint8_t source = 0;
        uint8_t destination = 0;
        uint16_t checksum = 0;
    };

    struct ItlMessage {
        ItlHeader header;
        std::vector<uint8_t> payload;
    };

    class ItlProtocol {
    public:
        explicit ItlProtocol(std::unique_ptr<TcpClientTransport> transport);
        ~ItlProtocol() = default;

        Result<void> connect() const;
        Result<void> disconnect() const;
        Result<std::vector<uint8_t>> sendPayload(uint32_t opcode, const std::vector<uint8_t>& payload) const;

    private:
        std::unique_ptr<TcpClientTransport> transport_;

        static std::vector<uint8_t> createMessage(uint32_t opcode, const std::vector<uint8_t>& payload);
        static std::vector<uint8_t> serialize(ItlMessage message);
        static std::vector<uint8_t> serializeHeader(const ItlHeader& header);
        static Result<ItlMessage> deserialize(const std::vector<uint8_t>& data);
        static void printMessage(const std::vector<uint8_t>& message);
        static uint16_t calculateXorChecksum(const std::vector<uint8_t>& data);
        static uint16_t calculateMessageChecksum(const ItlMessage& message);
        static bool isValidChecksum(const ItlMessage& message);
    };
}
