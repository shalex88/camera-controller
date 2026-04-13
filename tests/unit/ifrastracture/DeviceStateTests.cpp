#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

#include <gtest/gtest.h>

#include "infrastructure/camera/devices/FakeAdvancedCamera.h"
#include "infrastructure/camera/devices/FakeSimpleCamera.h"
#include "infrastructure/camera/devices/MwirCamera.h"
#include "infrastructure/camera/protocol/itl/ItlProtocol.h"
#include "infrastructure/camera/transport/ITransport.h"

using namespace service;

namespace {
    constexpr std::uint32_t kMwirDeviceId =
        static_cast<std::uint32_t>('E') |
        (static_cast<std::uint32_t>('N') << 8U) |
        (static_cast<std::uint32_t>('G') << 16U) |
        (static_cast<std::uint32_t>('2') << 24U);
    constexpr std::uint32_t kSetAoiIndexOpcode = 0x0000'000E;
    constexpr std::uint32_t kAckResponseOpcode = 0x9999'9999;
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

    std::vector<std::byte> makeAckWrappedItlResponse(const std::uint32_t request_opcode, const std::uint32_t device_id) {
        std::vector<std::byte> response;
        response.reserve(kItlHeaderSize + sizeof(request_opcode));

        const auto opcode = toBytes32(kAckResponseOpcode);

        const auto id = toBytes32(device_id);
        const auto length = toBytes16(static_cast<std::uint16_t>(kItlHeaderSize + sizeof(request_opcode)));
        const std::array<std::byte, 2> counter{std::byte{1}, std::byte{0}};
        const std::array<std::byte, 4> timestamp{};
        constexpr std::byte source{0};
        constexpr std::byte destination{0};
        const auto wrapped_opcode = toBytes32(request_opcode);

        response.insert(response.end(), opcode.begin(), opcode.end());
        response.insert(response.end(), id.begin(), id.end());
        response.insert(response.end(), length.begin(), length.end());
        response.insert(response.end(), counter.begin(), counter.end());
        response.insert(response.end(), timestamp.begin(), timestamp.end());
        response.push_back(source);
        response.push_back(destination);
        response.push_back(std::byte{0});
        response.push_back(std::byte{0});
        response.insert(response.end(), wrapped_opcode.begin(), wrapped_opcode.end());

        std::uint16_t checksum = 0;
        for (std::size_t i = 0; i < response.size(); ++i) {
            if (i == kItlHeaderSize - 2 || i == kItlHeaderSize - 1) {
                continue;
            }
            checksum ^= std::to_integer<std::uint8_t>(response[i]);
        }

        const auto checksum_bytes = toBytes16(checksum);
        response[kItlHeaderSize - 2] = checksum_bytes[0];
        response[kItlHeaderSize - 1] = checksum_bytes[1];

        return response;
    }

    class SuccessfulItlTransport final : public infrastructure::ITransport {
    public:
        SuccessfulItlTransport()
            : response_(makeAckWrappedItlResponse(kSetAoiIndexOpcode, kMwirDeviceId)) {
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

    std::unique_ptr<infrastructure::MwirCamera> makeMwirCamera() {
        auto transport = std::make_unique<SuccessfulItlTransport>();
        auto protocol = std::make_unique<infrastructure::ItlProtocol>(std::move(transport));
        return std::make_unique<infrastructure::MwirCamera>(std::move(protocol));
    }
} // namespace

TEST(DeviceStateTests, FakeAdvancedCameraStateIsPerInstance) {
    infrastructure::FakeAdvancedCamera first;
    infrastructure::FakeAdvancedCamera second;

    ASSERT_TRUE(first.enableAutoFocus(false).isSuccess());
    ASSERT_TRUE(first.setZoom(17).isSuccess());
    ASSERT_TRUE(first.setFocus(29).isSuccess());
    ASSERT_TRUE(first.stabilize(true).isSuccess());

    const auto first_zoom = first.getZoom();
    ASSERT_TRUE(first_zoom.isSuccess());
    EXPECT_EQ(first_zoom.value(), 17u);

    const auto second_zoom = second.getZoom();
    ASSERT_TRUE(second_zoom.isSuccess());
    EXPECT_EQ(second_zoom.value(), 0u);

    const auto second_auto_focus = second.isAutoFocusEnabled();
    ASSERT_TRUE(second_auto_focus.isSuccess());
    EXPECT_TRUE(second_auto_focus.value());

    const auto second_stabilization = second.isStabilizationEnabled();
    ASSERT_TRUE(second_stabilization.isSuccess());
    EXPECT_FALSE(second_stabilization.value());

    const auto second_focus = second.getFocus();
    ASSERT_TRUE(second_focus.isError());
}

TEST(DeviceStateTests, FakeSimpleCameraStateIsPerInstance) {
    infrastructure::FakeSimpleCamera first;
    infrastructure::FakeSimpleCamera second;

    ASSERT_TRUE(first.setZoom(11).isSuccess());
    ASSERT_TRUE(first.setFocus(22).isSuccess());

    const auto first_zoom = first.getZoom();
    ASSERT_TRUE(first_zoom.isSuccess());
    EXPECT_EQ(first_zoom.value(), 11u);

    const auto first_focus = first.getFocus();
    ASSERT_TRUE(first_focus.isSuccess());
    EXPECT_EQ(first_focus.value(), 22u);

    const auto second_zoom = second.getZoom();
    ASSERT_TRUE(second_zoom.isSuccess());
    EXPECT_EQ(second_zoom.value(), 0u);

    const auto second_focus = second.getFocus();
    ASSERT_TRUE(second_focus.isSuccess());
    EXPECT_EQ(second_focus.value(), 0u);
}

TEST(DeviceStateTests, MwirCameraAutofocusStateCanBeDisabled) {
    auto camera = makeMwirCamera();

    ASSERT_TRUE(camera->open().isSuccess());

    const auto initial_auto_focus = camera->isAutoFocusEnabled();
    ASSERT_TRUE(initial_auto_focus.isSuccess());
    EXPECT_TRUE(initial_auto_focus.value());

    ASSERT_TRUE(camera->enableAutoFocus(false).isSuccess());

    const auto disabled_auto_focus = camera->isAutoFocusEnabled();
    ASSERT_TRUE(disabled_auto_focus.isSuccess());
    EXPECT_FALSE(disabled_auto_focus.value());

    ASSERT_TRUE(camera->setFocus(9).isSuccess());

    const auto focus = camera->getFocus();
    ASSERT_TRUE(focus.isSuccess());
    EXPECT_EQ(focus.value(), 9u);
}

TEST(DeviceStateTests, MwirCameraStateIsPerInstance) {
    auto first = makeMwirCamera();
    auto second = makeMwirCamera();

    ASSERT_TRUE(first->setZoom(44).isSuccess());

    const auto first_zoom = first->getZoom();
    ASSERT_TRUE(first_zoom.isSuccess());
    EXPECT_EQ(first_zoom.value(), 44u);

    const auto second_zoom = second->getZoom();
    ASSERT_TRUE(second_zoom.isSuccess());
    EXPECT_EQ(second_zoom.value(), 0u);

    ASSERT_TRUE(first->enableAutoFocus(false).isSuccess());
    ASSERT_TRUE(first->setFocus(13).isSuccess());

    const auto first_focus = first->getFocus();
    ASSERT_TRUE(first_focus.isSuccess());
    EXPECT_EQ(first_focus.value(), 13u);

    const auto second_auto_focus = second->isAutoFocusEnabled();
    ASSERT_TRUE(second_auto_focus.isSuccess());
    EXPECT_TRUE(second_auto_focus.value());

    const auto second_focus = second->getFocus();
    ASSERT_TRUE(second_focus.isError());
}
