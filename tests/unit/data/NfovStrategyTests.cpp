#include <gtest/gtest.h>
#include <gmock/gmock.h>
/* Add your project include files here */
#include "data/NfovCamera.h"
#include "common/types/Result.h"

using namespace camera_service;
using namespace testing;

class NfovStrategyTests : public Test {
protected:
    void SetUp() override {
        camera_impl_ = std::make_unique<data::NfovCamera>();
    }

    std::unique_ptr<data::NfovCamera> camera_impl_;
};

TEST_F(NfovStrategyTests, CanBeConstructed) {
    const auto camera = std::make_unique<data::NfovCamera>();
    ASSERT_NE(nullptr, camera);
}

TEST_F(NfovStrategyTests, InitiallyNotConnected) {
    EXPECT_FALSE(camera_impl_->isConnected());
}

TEST_F(NfovStrategyTests, ConnectDisconnect) {
    const auto connect_result = camera_impl_->connect();
    EXPECT_TRUE(connect_result.isSuccess());
    EXPECT_TRUE(camera_impl_->isConnected());

    const auto disconnect_result = camera_impl_->disconnect();
    EXPECT_TRUE(disconnect_result.isSuccess());
    EXPECT_FALSE(camera_impl_->isConnected());
}

TEST_F(NfovStrategyTests, CanBeConnected) {
    const auto result = camera_impl_->connect();
    EXPECT_TRUE(result.isSuccess());
    EXPECT_TRUE(camera_impl_->isConnected());
}

TEST_F(NfovStrategyTests, DisconnectWhenNotConnected) {
    const auto result = camera_impl_->disconnect();
    EXPECT_TRUE(result.isSuccess());
    EXPECT_FALSE(camera_impl_->isConnected());
}

TEST_F(NfovStrategyTests, ZoomOperations) {
    constexpr auto expected_value = 2.0;

    const auto set_result = camera_impl_->setZoom(expected_value);
    EXPECT_TRUE(set_result.isSuccess());

    const auto get_result = camera_impl_->getZoom();
    ASSERT_TRUE(get_result.isSuccess());
    EXPECT_DOUBLE_EQ(expected_value, get_result.value());
}

TEST_F(NfovStrategyTests, FocusOperations) {
    constexpr auto expected_value = 2.0;

    const auto set_result = camera_impl_->setFocus(expected_value);
    EXPECT_TRUE(set_result.isSuccess());

    const auto get_result = camera_impl_->getFocus();
    ASSERT_TRUE(get_result.isSuccess());
    EXPECT_DOUBLE_EQ(expected_value, get_result.value());
}

TEST_F(NfovStrategyTests, InvalidZoomValue) {
    const auto result = camera_impl_->setZoom(-1.0);
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error(), "Invalid zoom level: Value must be greater than zero");
}
