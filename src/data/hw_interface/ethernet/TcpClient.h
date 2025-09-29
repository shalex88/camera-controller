#pragma once

#include <string>
#include <vector>

#include "common/types/Result.h"

namespace camera_service::data {
    struct ItlHeader;
    struct ItlMessage;

    class TcpClient {
    public:
        explicit TcpClient(const std::string& device_path);
        ~TcpClient();

        Result<void> connect();
        Result<void> disconnect();
        Result<std::vector<uint8_t>> sendPayload(uint32_t opcode, const std::vector<uint8_t>& payload) const;

    private:
        std::string ip_;
        uint16_t port_ = 0;
        int socket_fd_ = -1;
        bool is_connected_ = false;

        Result<void> send(const std::vector<uint8_t>& payload) const;
        Result<std::vector<uint8_t>> receive() const;
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