#include <gtest/gtest.h>
#include <grpcpp/grpcpp.h>
/* Add your project include files here */
#include <chrono>
#include <memory>
/* Add your project include files here */
#include "api/ControllerFactory.h"
#include "api/proto/camera_service.grpc.pb.h"
#include "api/proto/camera_service.pb.h"
#include "core/CoreFactory.h"
#include "data/CameraFactory.h"
#include "common/Config/Config.h"

using namespace camera_service;
using namespace testing;
using namespace std::chrono_literals;

class CameraServiceClient {
public:
    explicit CameraServiceClient(const std::shared_ptr<grpc::Channel>& channel)
        : stub_(camera::CameraService::NewStub(channel)) {}

    bool SetZoom(double zoom_value, const std::chrono::milliseconds timeout = 300ms) const {
        camera::SetZoomRequest request;
        camera::SetZoomResponse response;
        grpc::ClientContext context;

        request.set_zoom(zoom_value);
        context.set_deadline(std::chrono::system_clock::now() + timeout);

        const grpc::Status status = stub_->SetZoom(&context, request, &response);
        return status.ok();
    }

    std::optional<double> GetZoom(const std::chrono::milliseconds timeout = 300ms) const {
        camera::GetZoomRequest request;
        camera::GetZoomResponse response;
        grpc::ClientContext context;

        context.set_deadline(std::chrono::system_clock::now() + timeout);

        const grpc::Status status = stub_->GetZoom(&context, request, &response);
        if (!status.ok()) {
            return std::nullopt;
        }
        return response.zoom();
    }

    bool SetFocus(double focus_value, const std::chrono::milliseconds timeout = 300ms) const {
        camera::SetFocusRequest request;
        camera::SetFocusResponse response;
        grpc::ClientContext context;

        request.set_focus(focus_value);
        context.set_deadline(std::chrono::system_clock::now() + timeout);

        const grpc::Status status = stub_->SetFocus(&context, request, &response);
        return status.ok();
    }

    std::optional<double> GetFocus(const std::chrono::milliseconds timeout = 300ms) const {
        camera::GetFocusRequest request;
        camera::GetFocusResponse response;
        grpc::ClientContext context;

        context.set_deadline(std::chrono::system_clock::now() + timeout);

        const grpc::Status status = stub_->GetFocus(&context, request, &response);
        if (!status.ok()) {
            return std::nullopt;
        }
        return response.focus();
    }

private:
    std::unique_ptr<camera::CameraService::Stub> stub_;
};

class ServiceSystemTests : public Test {
protected:
    void SetUp() override {
        EXPECT_NO_THROW(config = std::make_unique<Config>("../../config/config.yaml"));
        ASSERT_NE(nullptr, config);
        EXPECT_NO_THROW(api_config = config->get("api"));
        EXPECT_NO_THROW(port_config = config->get("port"));
        EXPECT_NO_THROW(camera_config = config->get("camera"));

        EXPECT_NO_THROW(camera = data::CameraFactory::createCamera(camera_config));
        ASSERT_NE(nullptr, camera);

        EXPECT_NO_THROW(core = core::CoreFactory::createCore(camera_config, std::move(camera)));
        ASSERT_NE(nullptr, core);

        EXPECT_NO_THROW(controller = camera_service::api::ControllerFactory::createController(api_config, port_config,
            std::move(core)));
        ASSERT_NE(nullptr, controller);

        EXPECT_TRUE(controller->startAsync());
        std::this_thread::sleep_for(1s);
        EXPECT_TRUE(controller->isRunning());
    }

    std::unique_ptr<Config> config;
    std::unique_ptr<data::ICamera> camera;
    std::unique_ptr<core::ICore> core;
    std::unique_ptr<api::IController> controller;
    std::string api_config;
    std::string port_config;
    std::string camera_config;
};

TEST_F(ServiceSystemTests, CameraRequestResponse) {
    const auto server_address = "localhost:" + port_config;
    std::cout << "Connecting to server at " << server_address << std::endl;
    auto channel = grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials());
    CameraServiceClient client(channel);

    constexpr double test_zoom = 2.5;
    std::cout << "Test SetZoom " << test_zoom <<" and GetZoom" << std::endl;
    EXPECT_TRUE(client.SetZoom(test_zoom));

    auto zoom_result = client.GetZoom();
    ASSERT_TRUE(zoom_result.has_value());
    EXPECT_DOUBLE_EQ(test_zoom, zoom_result.value());

    constexpr double test_focus = 1.8;
    std::cout << "Test SetFocus " << test_focus <<" and GetFocus" << std::endl;
    EXPECT_TRUE(client.SetFocus(test_focus));

    auto focus_result = client.GetFocus();
    ASSERT_TRUE(focus_result.has_value());
    EXPECT_DOUBLE_EQ(test_focus, focus_result.value());
}

TEST_F(ServiceSystemTests, CollectCameraStatus) {
    FAIL() << "Not implemented";
}

TEST_F(ServiceSystemTests, CollectLogs) {
    FAIL() << "Not implemented";
}
