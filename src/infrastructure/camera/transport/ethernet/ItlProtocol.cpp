#include "ItlProtocol.h"

#include <cstdio>

namespace camera_service::infrastructure {
    ItlProtocol::ItlProtocol(std::unique_ptr<TcpClient> transport)
        : transport_(std::move(transport)) {
    }

    Result<void> ItlProtocol::connect() const {
        return transport_->connect();
    }

    Result<void> ItlProtocol::disconnect() const {
        return transport_->disconnect();
    }

    Result<std::vector<uint8_t>> ItlProtocol::sendPayload(
        const uint32_t opcode, const std::vector<uint8_t>& payload) const {
        const auto message = createMessage(opcode, payload);
        const auto send_result = transport_->send(message);
        if (send_result.isError()) {
            return Result<std::vector<uint8_t>>::error(send_result.error());
        }

        const auto serialized_response = transport_->receive();
        if (serialized_response.isError()) {
            return Result<std::vector<uint8_t>>::error(serialized_response.error());
        }

        const auto deserialized_response = deserialize(serialized_response.value());
        if (deserialized_response.isError()) {
            return Result<std::vector<uint8_t>>::error(deserialized_response.error());
        }

        return Result<std::vector<uint8_t>>::success(deserialized_response.value().payload);
    }

    std::vector<uint8_t> ItlProtocol::createMessage(const uint32_t opcode, const std::vector<uint8_t>& payload) {
        ItlMessage message;
        message.payload = payload;

        message.header.opcode = opcode;
        message.header.length = static_cast<uint16_t>(sizeof(message.header) + payload.size());
        message.header.checksum = calculateMessageChecksum(message);

        printMessage(serialize(message));

        return serialize(message);
    }

    std::vector<uint8_t> ItlProtocol::serialize(ItlMessage message) {
        auto serialized_message = serializeHeader(message.header);
        serialized_message.insert(serialized_message.end(), message.payload.begin(), message.payload.end());

        return serialized_message;
    }

    std::vector<uint8_t> ItlProtocol::serializeHeader(const ItlHeader& header) {
        std::vector<uint8_t> serialized_header;

        for (int i = 0; i < sizeof(header.opcode); ++i) {
            serialized_header.push_back(static_cast<uint8_t>((header.opcode >> (i * 8)) & 0xFF));
        }

        for (int i = 0; i < sizeof(header.id); ++i) {
            serialized_header.push_back(header.id[i]);
        }

        for (int i = 0; i < sizeof(header.length); ++i) {
            serialized_header.push_back(static_cast<uint8_t>((header.length >> (i * 8)) & 0xFF));
        }

        for (int i = 0; i < sizeof(header.counter); ++i) {
            serialized_header.push_back(static_cast<uint8_t>((header.counter >> (i * 8)) & 0xFF));
        }

        for (int i = 0; i < sizeof(header.time_stamp); ++i) {
            serialized_header.push_back(static_cast<uint8_t>((header.time_stamp >> (i * 8)) & 0xFF));
        }

        serialized_header.push_back(header.source);

        serialized_header.push_back(header.destination);

        for (int i = 0; i < sizeof(header.checksum); ++i) {
            serialized_header.push_back(static_cast<uint8_t>((header.checksum >> (i * 8)) & 0xFF));
        }

        return serialized_header;
    }

    Result<ItlMessage> ItlProtocol::deserialize(const std::vector<uint8_t>& data) {
        ItlMessage message;
        if (data.size() < sizeof(ItlHeader)) {
            return Result<ItlMessage>::error("Data too short to contain valid header");
        }

        size_t offset = 0;

        for (int i = 0; i < sizeof(message.header.opcode); ++i) {
            message.header.opcode |= static_cast<uint32_t>(data[offset++]) << (i * 8);
        }
        //TODO: validate received opcode is (xFO | sent opcode)
        if (data[3] != 0xF0) {
            return Result<ItlMessage>::error("Invalid opcode in response");
        }

        for (int i = 0; i < sizeof(message.header.id); ++i) {
            message.header.id[i] = data[offset++];
        }

        for (int i = 0; i < sizeof(message.header.length); ++i) {
            message.header.length |= static_cast<uint16_t>(data[offset++]) << (i * 8);
        }
        if (message.header.length != data.size()) {
            return Result<ItlMessage>::error("Length field does not match actual data size");
        }

        for (int i = 0; i < sizeof(message.header.counter); ++i) {
            message.header.counter |= static_cast<uint16_t>(data[offset++]) << (i * 8);
        }

        for (int i = 0; i < sizeof(message.header.time_stamp); ++i) {
            message.header.time_stamp |= static_cast<uint32_t>(data[offset++]) << (i * 8);
        }

        message.header.source = data[offset++];

        message.header.destination = data[offset++];

        for (int i = 0; i < sizeof(message.header.checksum); ++i) {
            message.header.checksum |= static_cast<uint16_t>(data[offset++]) << (i * 8);
        }

        if (offset < data.size()) {
            message.payload.insert(message.payload.end(), data.begin() + offset, data.end());
        }

        if (!isValidChecksum(message)) {
            return Result<ItlMessage>::error("Received message has invalid checksum");
        }

        return Result<ItlMessage>::success(message);
    }

    void ItlProtocol::printMessage(const std::vector<uint8_t>& message) {
        std::string buffer = "Message (" + std::to_string(message.size()) + " bytes): [";
        for (size_t i = 0; i < message.size(); ++i) {
            if (i > 0) buffer += " ";
            char hex_buffer[3];
            snprintf(hex_buffer, sizeof(hex_buffer), "%02x", message[i]);
            buffer += hex_buffer;
        }
        buffer += "]";
        LOG_DEBUG("{}", buffer); //TODO: use layered logger
    }

    uint16_t ItlProtocol::calculateXorChecksum(const std::vector<uint8_t>& data) {
        uint16_t checksum = 0;
        for (const auto byte : data) {
            checksum ^= byte;
        }
        return checksum;
    }

    uint16_t ItlProtocol::calculateMessageChecksum(const ItlMessage& message) {
        const ItlHeader temp_header = message.header;

        auto data_for_checksum = serializeHeader(temp_header);
        data_for_checksum.resize(data_for_checksum.size() - 2);
        data_for_checksum.insert(data_for_checksum.end(), message.payload.begin(), message.payload.end());

        return calculateXorChecksum(data_for_checksum);
    }

    bool ItlProtocol::isValidChecksum(const ItlMessage& message) {
        ItlMessage temp_message = message;
        temp_message.header.checksum = 0;

        return calculateMessageChecksum(temp_message) == message.header.checksum;
    }
}
