#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "infrastructure/camera/protocol/itl/ItlProtocol.h"
#include "infrastructure/camera/transport/ITransport.h"

using namespace service;
using testing::HasSubstr;

namespace {
    constexpr std::uint32_t kDeviceId =
        static_cast<std::uint32_t>('E') |
        (static_cast<std::uint32_t>('N') << 8U) |
        (static_cast<std::uint32_t>('G') << 16U) |
        (static_cast<std::uint32_t>('2') << 24U);
    constexpr std::uint32_t kGetVersionOpcode = 0x0000'0001;
    constexpr std::uint32_t kSetAoiIndexOpcode = 0x0000'000E;
    constexpr std::uint32_t kAckResponseOpcode = 0x9999'9999;
    constexpr std::uint32_t kNackResponseOpcode = 0x8888'8888;
    constexpr std::uint32_t kErrorResponseOpcode = 0x7777'7777;
    constexpr std::size_t kItlHeaderSize = 20;

    std::array<std::byte, 2> toBytes16(const std::uint16_t value) {
        return {
            static_cast<std::byte>(value & 0xFF),
            static_cast<std::byte>((value >> 8U) & 0xFF)
        };
    }

    std::array<std::byte, 4> toBytes32(const std::uint32_t value) {
        return {
            static_cast<std::byte>(value & 0xFF),
            static_cast<std::byte>((value >> 8U) & 0xFF),
            static_cast<std::byte>((value >> 16U) & 0xFF),
            static_cast<std::byte>((value >> 24U) & 0xFF)
        };
    }

    std::uint16_t calculateChecksum(std::span<const std::byte> message) {
        std::uint16_t checksum = 0;
        for (std::size_t i = 0; i < message.size(); ++i) {
            if (i == kItlHeaderSize - 2 || i == kItlHeaderSize - 1) {
                continue;
            }
            checksum ^= std::to_integer<std::uint8_t>(message[i]);
        }
        return checksum;
    }

    std::uint32_t makeStandardResponseOpcode(const std::uint32_t request_opcode) {
        auto response_opcode = toBytes32(request_opcode);
        response_opcode.back() = std::byte{0xF0};
        return std::to_integer<std::uint32_t>(response_opcode[0]) |
            (std::to_integer<std::uint32_t>(response_opcode[1]) << 8U) |
            (std::to_integer<std::uint32_t>(response_opcode[2]) << 16U) |
            (std::to_integer<std::uint32_t>(response_opcode[3]) << 24U);
    }

    std::vector<std::byte> makeWrappedPayload(const std::uint32_t opcode, std::span<const std::byte> suffix = {}) {
        auto payload = std::vector<std::byte>{};
        const auto opcode_bytes = toBytes32(opcode);
        payload.insert(payload.end(), opcode_bytes.begin(), opcode_bytes.end());
        payload.insert(payload.end(), suffix.begin(), suffix.end());
        return payload;
    }

    std::vector<std::byte> makeErrorPayload(const std::uint32_t opcode, const std::uint16_t reason,
                                            std::span<const std::byte> suffix = {}) {
        auto payload = makeWrappedPayload(opcode);
        const auto reason_bytes = toBytes16(reason);
        payload.insert(payload.end(), reason_bytes.begin(), reason_bytes.end());
        payload.insert(payload.end(), suffix.begin(), suffix.end());
        return payload;
    }

    std::vector<std::byte> makeItlResponse(const std::uint32_t response_opcode, const std::uint32_t device_id,
                                           std::span<const std::byte> payload = {}) {
        std::vector<std::byte> response;
        response.reserve(kItlHeaderSize + payload.size());

        const auto opcode = toBytes32(response_opcode);
        const auto id = toBytes32(device_id);
        const auto length = toBytes16(static_cast<std::uint16_t>(kItlHeaderSize + payload.size()));
        const std::array<std::byte, 2> counter{std::byte{1}, std::byte{0}};
        const std::array<std::byte, 4> timestamp{};

        response.insert(response.end(), opcode.begin(), opcode.end());
        response.insert(response.end(), id.begin(), id.end());
        response.insert(response.end(), length.begin(), length.end());
        response.insert(response.end(), counter.begin(), counter.end());
        response.insert(response.end(), timestamp.begin(), timestamp.end());
        response.push_back(std::byte{0});
        response.push_back(std::byte{0});
        response.push_back(std::byte{0});
        response.push_back(std::byte{0});
        response.insert(response.end(), payload.begin(), payload.end());

        const auto checksum_bytes = toBytes16(calculateChecksum(response));
        response[kItlHeaderSize - 2] = checksum_bytes[0];
        response[kItlHeaderSize - 1] = checksum_bytes[1];

        return response;
    }

    class StaticResponseTransport final : public infrastructure::ITransport {
    public:
        explicit StaticResponseTransport(std::vector<std::byte> response)
            : response_(std::move(response)) {
        }

        Result<void> open() override {
            is_open_ = true;
            return Result<void>::success();
        }

        Result<void> close() override {
            is_open_ = false;
            return Result<void>::success();
        }

        Result<void> write(std::span<const std::byte>) override {
            return Result<void>::success();
        }

        Result<size_t> read(std::span<std::byte> buffer) override {
            if (buffer.size() < response_.size()) {
                return Result<size_t>::error("Buffer too small");
            }

            std::copy(response_.begin(), response_.end(), buffer.begin());
            return Result<size_t>::success(response_.size());
        }

        bool isOpen() const override {
            return is_open_;
        }

    private:
        bool is_open_{true};
        std::vector<std::byte> response_;
    };

    std::unique_ptr<infrastructure::ItlProtocol> makeProtocol(std::vector<std::byte> response) {
        return std::make_unique<infrastructure::ItlProtocol>(
            std::make_unique<StaticResponseTransport>(std::move(response)));
    }
} // namespace

TEST(ItlProtocolTests, StandardResponseReturnsPayload) {
    const std::array<std::byte, 4> payload{
        std::byte{0x45}, std::byte{0xF2}, std::byte{0x3F}, std::byte{0x45}
    };
    auto protocol = makeProtocol(makeItlResponse(
        makeStandardResponseOpcode(kGetVersionOpcode),
        kDeviceId,
        std::span<const std::byte>{payload}));

    const auto result = protocol->send(kDeviceId, kGetVersionOpcode);

    ASSERT_TRUE(result.isSuccess());
    EXPECT_EQ(result.value(), std::vector<std::byte>(payload.begin(), payload.end()));
}

TEST(ItlProtocolTests, AckWrappedResponseReturnsPayloadWithoutEchoedOpcode) {
    const std::array<std::byte, 2> wrapped_payload{std::byte{0xAA}, std::byte{0x55}};
    const auto response = makeItlResponse(
        kAckResponseOpcode,
        kDeviceId,
        makeWrappedPayload(kSetAoiIndexOpcode, std::span<const std::byte>{wrapped_payload}));
    auto protocol = makeProtocol(response);

    const auto result = protocol->send(kDeviceId, kSetAoiIndexOpcode, std::span<const std::byte>{wrapped_payload});

    ASSERT_TRUE(result.isSuccess());
    EXPECT_EQ(result.value(), std::vector<std::byte>(wrapped_payload.begin(), wrapped_payload.end()));
}

TEST(ItlProtocolTests, NackWrappedResponseReturnsDeviceError) {
    auto protocol = makeProtocol(makeItlResponse(
        kNackResponseOpcode,
        kDeviceId,
        makeWrappedPayload(kSetAoiIndexOpcode)));

    const auto result = protocol->send(kDeviceId, kSetAoiIndexOpcode);

    ASSERT_TRUE(result.isError());
    EXPECT_THAT(result.error(), HasSubstr("Device NACK"));
    EXPECT_THAT(result.error(), HasSubstr("0x0000000E"));
}

TEST(ItlProtocolTests, ErrorWrappedResponseReturnsReasonCode) {
    auto protocol = makeProtocol(makeItlResponse(
        kErrorResponseOpcode,
        kDeviceId,
        makeErrorPayload(kSetAoiIndexOpcode, 0x1234)));

    const auto result = protocol->send(kDeviceId, kSetAoiIndexOpcode);

    ASSERT_TRUE(result.isError());
    EXPECT_THAT(result.error(), HasSubstr("Device ERROR"));
    EXPECT_THAT(result.error(), HasSubstr("0x0000000E"));
    EXPECT_THAT(result.error(), HasSubstr("0x1234"));
}

TEST(ItlProtocolTests, WrappedResponseWithUnexpectedOpcodeIsRejected) {
    auto protocol = makeProtocol(makeItlResponse(
        kAckResponseOpcode,
        kDeviceId,
        makeWrappedPayload(kGetVersionOpcode)));

    const auto result = protocol->send(kDeviceId, kSetAoiIndexOpcode);

    ASSERT_TRUE(result.isError());
    EXPECT_THAT(result.error(), HasSubstr("Invalid response received"));
    EXPECT_THAT(result.error(), HasSubstr("Wrapped response opcode does not match request"));
}

TEST(ItlProtocolTests, ErrorWrappedResponseWithoutReasonIsRejected) {
    auto protocol = makeProtocol(makeItlResponse(
        kErrorResponseOpcode,
        kDeviceId,
        makeWrappedPayload(kSetAoiIndexOpcode)));

    const auto result = protocol->send(kDeviceId, kSetAoiIndexOpcode);

    ASSERT_TRUE(result.isError());
    EXPECT_THAT(result.error(), HasSubstr("Invalid response received"));
    EXPECT_THAT(result.error(), HasSubstr("ERROR response is missing reason code"));
}

TEST(ItlProtocolTests, ResponseWithInvalidChecksumIsRejected) {
    auto response = makeItlResponse(
        makeStandardResponseOpcode(kGetVersionOpcode),
        kDeviceId);
    response[kItlHeaderSize - 2] ^= std::byte{0x01};
    auto protocol = makeProtocol(std::move(response));

    const auto result = protocol->send(kDeviceId, kGetVersionOpcode);

    ASSERT_TRUE(result.isError());
    EXPECT_THAT(result.error(), HasSubstr("Invalid response received: Invalid checksum"));
}

TEST(ItlProtocolTests, ResponseWithLengthLargerThanBufferIsRejected) {
    auto response = makeItlResponse(
        makeStandardResponseOpcode(kGetVersionOpcode),
        kDeviceId);
    const auto invalid_length = toBytes16(static_cast<std::uint16_t>(kItlHeaderSize + 4));
    response[8] = invalid_length[0];
    response[9] = invalid_length[1];
    auto protocol = makeProtocol(std::move(response));

    const auto result = protocol->send(kDeviceId, kGetVersionOpcode);

    ASSERT_TRUE(result.isError());
    EXPECT_THAT(result.error(), HasSubstr("Invalid response received: Declared length exceeds received data"));
}
