#pragma once

#include "api/proto/camera_service.pb.h"
#include "api/proto/camera_service.grpc.pb.h"
#include "api/IApiAdapter.h"

namespace camera_service::api {
    class IController;
    class GrpcAdapter final : public camera::CameraService::CallbackService, public IApiAdapter {
    public:
        explicit GrpcAdapter();
        void setController(IController* controller) override;
        ~GrpcAdapter() override;

        bool start(const std::string& port) override;
        void stop() override;

        grpc::ServerUnaryReactor* SetZoom(
            grpc::CallbackServerContext* context,
            const camera::SetZoomRequest* request,
            camera::SetZoomResponse* response) override;

        grpc::ServerUnaryReactor* SetFocus(
            grpc::CallbackServerContext* context,
            const camera::SetFocusRequest* request,
            camera::SetFocusResponse* response) override;

        grpc::ServerUnaryReactor* GetZoom(
            grpc::CallbackServerContext* context,
            const camera::GetZoomRequest* request,
            camera::GetZoomResponse* response) override;

        grpc::ServerUnaryReactor* GetFocus(
            grpc::CallbackServerContext* context,
            const camera::GetFocusRequest* request,
            camera::GetFocusResponse* response) override;

        void runLoop() override;

    private:
        IController* controller_;
        std::unique_ptr<grpc::Server> server_;
    };
}
