#include "ItlProtocol.h"

#include <algorithm>
#include <string>

#include "infrastructure/camera/transport/ITransport.h"

namespace {
    struct __attribute__((packed)) ItlHeader {
        std::array<std::byte, 4> opcode{};
        std::array<std::byte, 4> id{};
        std::array<std::byte, 2> length{};
        std::array<std::byte, 2> counter{std::byte{1}, std::byte{0}};
        std::array<std::byte, 4> time_stamp{};
        std::byte source{2};
        std::byte destination{1};
        std::array<std::byte, 2> checksum{};
    };

    struct ItlMessage {
        ItlHeader header;
        std::vector<std::byte> payload;
    };

    std::array<std::byte, 2> toBytes(std::uint16_t value) {
        return {static_cast<std::byte>(value & 0xFF), static_cast<std::byte>((value >> 8) & 0xFF)};
    }

    std::array<std::byte, 4> toBytes(std::uint32_t value) {
        constexpr std::uint32_t byte_mask = 0xFF;
        constexpr std::uint32_t bits_per_byte = 8;
        return {
            static_cast<std::byte>(value & byte_mask), static_cast<std::byte>((value >> bits_per_byte) & byte_mask),
            static_cast<std::byte>((value >> (2 * bits_per_byte)) & byte_mask),
            static_cast<std::byte>((value >> (3 * bits_per_byte)) & byte_mask)
        };
    }

    std::uint16_t fromBytes(std::span<const std::byte, 2> bytes) {
        return static_cast<std::uint16_t>(bytes[0]) | (static_cast<std::uint16_t>(bytes[1]) << 8);
    }

    std::array<std::byte, 4> toResponseOpcodeBytes(std::uint32_t request_opcode) {
        auto response_opcode = toBytes(request_opcode);
        response_opcode.back() = std::byte{0xF0};
        return response_opcode;
    }

    std::vector<std::byte> serializeHeader(const ItlHeader& header) {
        std::vector<std::byte> serialized_header;
        serialized_header.reserve(sizeof(ItlHeader));

        serialized_header.insert(serialized_header.end(), header.opcode.begin(), header.opcode.end());
        serialized_header.insert(serialized_header.end(), header.id.begin(), header.id.end());
        serialized_header.insert(serialized_header.end(), header.length.begin(), header.length.end());
        serialized_header.insert(serialized_header.end(), header.counter.begin(), header.counter.end());
        serialized_header.insert(serialized_header.end(), header.time_stamp.begin(), header.time_stamp.end());
        serialized_header.push_back(header.source);
        serialized_header.push_back(header.destination);
        serialized_header.insert(serialized_header.end(), header.checksum.begin(), header.checksum.end());

        return serialized_header;
    }

    std::vector<std::byte> serialize(const ItlMessage& message) {
        auto serialized_message = serializeHeader(message.header);
        serialized_message.insert(serialized_message.end(), message.payload.begin(), message.payload.end());
        return serialized_message;
    }

    std::array<std::byte, 2> calculateXorChecksum(std::span<const std::byte> data) {
        std::uint16_t checksum = 0;
        for (const auto byte : data) {
            checksum ^= static_cast<uint8_t>(byte);
        }
        return toBytes(checksum);
    }

    std::array<std::byte, 2> calculateMessageChecksum(const ItlMessage& message) {
        auto [header, payload] = message;
        header.checksum = {std::byte{0}, std::byte{0}};

        auto data_for_checksum = serializeHeader(header);
        data_for_checksum.resize(data_for_checksum.size() - 2);
        data_for_checksum.insert(data_for_checksum.end(), message.payload.begin(), message.payload.end());

        return calculateXorChecksum(data_for_checksum);
    }

    bool isValidChecksum(const ItlMessage& message) {
        const auto calculated = calculateMessageChecksum(message);
        return calculated == message.header.checksum;
    }

    bool isValidOpcode(std::span<const std::byte, 4> actual, std::uint32_t expected) {
        const auto expected_bytes = toResponseOpcodeBytes(expected);
        return std::equal(actual.begin(), actual.end(), expected_bytes.begin());
    }

    bool isValidId(std::span<const std::byte, 4> actual, std::uint32_t expected) {
        const auto expected_bytes = toBytes(expected);
        return std::equal(actual.begin(), actual.end(), expected_bytes.begin());
    }


    void printMessage(std::string_view label, std::span<const std::byte> message) {
        std::string buffer = std::string(label) + " (" + std::to_string(message.size()) + " bytes): [";
        for (size_t i = 0; i < message.size(); ++i) {
            if (i > 0) {
                buffer += " ";
            }
            char hex_buffer[3];
            snprintf(hex_buffer, sizeof(hex_buffer), "%02x", static_cast<uint8_t>(message[i]));
            buffer += hex_buffer;
        }
        buffer += "]";
        LOG_DEBUG("{}", buffer);
    }

    Result<ItlMessage> deserialize(std::span<const std::byte> data, std::uint32_t request_opcode,
                                   std::uint32_t device_id) {
        ItlMessage message;
        if (data.size() < sizeof(ItlHeader)) {
            return Result<ItlMessage>::error("Data is too short");
        }

        size_t offset = 0;

        // Opcode
        std::copy_n(data.begin() + offset, sizeof(message.header.opcode), message.header.opcode.begin());
        offset += sizeof(message.header.opcode);

        if (!isValidOpcode(message.header.opcode, request_opcode)) {
            return Result<ItlMessage>::error("Unexpected opcode");
        }

        // ID
        std::copy_n(data.begin() + offset, sizeof(message.header.id), message.header.id.begin());
        offset += sizeof(message.header.id);

        if (!isValidId(message.header.id, device_id)) {
            return Result<ItlMessage>::error("Unexpected ID");
        }

        // Length
        std::copy_n(data.begin() + offset, sizeof(message.header.length), message.header.length.begin());
        offset += sizeof(message.header.length);

        const std::uint16_t length = fromBytes(message.header.length);

        // Counter
        std::copy_n(data.begin() + offset, sizeof(message.header.counter), message.header.counter.begin());
        offset += sizeof(message.header.counter);

        // Time stamp
        std::copy_n(data.begin() + offset, sizeof(message.header.time_stamp), message.header.time_stamp.begin());
        offset += sizeof(message.header.time_stamp);

        // Source (1 byte)
        message.header.source = data[offset++];

        // Destination (1 byte)
        message.header.destination = data[offset++];

        // Checksum
        std::copy_n(data.begin() + offset, sizeof(message.header.checksum), message.header.checksum.begin());
        offset += sizeof(message.header.checksum);

        // Payload (remaining bytes)
        if (offset < length) {
            message.payload.insert(message.payload.end(), data.begin() + offset, data.end());
        }

        if (!isValidChecksum(message)) {
            return Result<ItlMessage>::error("Invalid checksum");
        }

        return Result<ItlMessage>::success(message);
    }

    std::vector<std::byte> createMessage(std::uint32_t opcode, std::span<const std::byte> payload,
                                         std::uint32_t message_id) {
        ItlMessage message;
        message.payload.assign(payload.begin(), payload.end());
        message.header.opcode = toBytes(opcode);
        message.header.id = toBytes(message_id);

        const std::uint16_t total_length = sizeof(message.header) + payload.size();
        message.header.length = toBytes(total_length);
        message.header.checksum = calculateMessageChecksum(message);

        auto serialized = serialize(message);
        printMessage("Request", serialized);

        return serialized;
    }
} // unnamed namespace

namespace service::infrastructure {
    ItlProtocol::ItlProtocol(std::unique_ptr<ITransport> transport) : transport_(std::move(transport)) {
    }

    ItlProtocol::~ItlProtocol() = default;

    Result<void> ItlProtocol::open() const {
        return transport_->open();
    }

    Result<void> ItlProtocol::close() const {
        return transport_->close();
    }

    Result<std::vector<std::byte>> ItlProtocol::send(std::uint32_t device_id, std::uint32_t opcode) const {
        return send(device_id, opcode, std::span<const std::byte>{});
    }

    Result<std::vector<std::byte>> ItlProtocol::send(std::uint32_t device_id, std::uint32_t opcode, std::span<const std::byte> payload) const {
        const auto message = createMessage(opcode, payload, device_id);
        if (const auto send_result = transport_->write(message); send_result.isError()) {
            return Result<std::vector<std::byte>>::error(send_result.error());
        }

        const auto serialized_response = transport_->read(rx_buffer_);
        if (serialized_response.isError()) {
            return Result<std::vector<std::byte>>::error(serialized_response.error());
        }

        const auto response_data = std::span<const std::byte>{rx_buffer_.data(), serialized_response.value()};
        printMessage("Received", response_data);
        const auto deserialized_response = deserialize(response_data, opcode, device_id);
        if (deserialized_response.isError()) {
            return Result<std::vector<std::byte>>::error("Invalid response received: " + deserialized_response.error());
        }

        return Result<std::vector<std::byte>>::success(deserialized_response.value().payload);
    }
} // namespace service::infrastructure
