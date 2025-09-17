#include <gtest/gtest.h>
#include <gmock/gmock.h>
/* Add your project include files here */
#include "common/Config/ConfigManager.h"
#include "data/CameraHal.h"
#include "data/camera/AdimecCamera.h"
#include "data/hw_interface/mmio/RegistersMapManager.h"
#include "data/hw_interface/mmio/RegisterImplUio.h"
#include "common/types/Result.h"

using namespace camera_service;
using namespace testing;

class AdimecCameraTests : public Test {
protected:
    AdimecCameraTests() : config_(std::make_unique<ConfigManager>("../../config/config-nfov.yaml")) {
        auto register_impl = std::make_unique<data::RegisterImplUio>(config_->getDataConfig().device);
        auto registers_manager = std::make_unique<data::RegistersMapManager>(std::move(register_impl));
        auto camera_hw = std::make_unique<data::AdimecCamera>(std::move(registers_manager));
        auto logger = std::make_shared<LayerLogger>(std::make_shared<SpdLogAdapter>(), "");
        camera_ = std::make_unique<data::CameraHal>(std::move(camera_hw), std::move(logger));
    }

    std::unique_ptr<data::CameraHal> camera_;
    std::unique_ptr<ConfigManager> config_;
};

TEST_F(AdimecCameraTests, CanBeConstructed) {
    auto register_impl = std::make_unique<data::RegisterImplUio>(config_->getDataConfig().device);
    auto registers_manager = std::make_unique<data::RegistersMapManager>(std::move(register_impl));
    const auto camera = std::make_unique<data::AdimecCamera>(std::move(registers_manager));
    ASSERT_NE(nullptr, camera);
}

TEST_F(AdimecCameraTests, ConnectDisconnect) {
    const auto connect_result = camera_->connect();
    ASSERT_TRUE(connect_result.isSuccess());

    const auto disconnect_result = camera_->disconnect();
    ASSERT_TRUE(disconnect_result.isSuccess());
}

TEST_F(AdimecCameraTests, CanBeConnected) {
    const auto result = camera_->connect();
    ASSERT_TRUE(result.isSuccess());
}

TEST_F(AdimecCameraTests, ErrorOnDisconnectWhenNotConnected) {
    const auto result = camera_->disconnect();
    ASSERT_TRUE(result.isError());
}

TEST_F(AdimecCameraTests, ZoomOperations) {
    const auto result = camera_->connect();
    ASSERT_TRUE(result.isSuccess());

    constexpr auto expected_value = 2u;

    const auto set_result = camera_->setZoom(expected_value);
    ASSERT_TRUE(set_result.isSuccess());

    const auto get_result = camera_->getZoom();
    ASSERT_TRUE(get_result.isSuccess());
    EXPECT_EQ(expected_value, get_result.value());
}

TEST_F(AdimecCameraTests, FocusOperations) {
    const auto result = camera_->connect();
    ASSERT_TRUE(result.isSuccess());

    constexpr auto expected_value = 2u;

    const auto set_result = camera_->setFocus(expected_value);
    ASSERT_TRUE(set_result.isSuccess());

    const auto get_result = camera_->getFocus();
    ASSERT_TRUE(get_result.isSuccess());
    EXPECT_EQ(expected_value, get_result.value());
}