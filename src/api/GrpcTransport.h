#pragma once

#include "api/proto/camera_service.pb.h"
#include "api/proto/camera_service.grpc.pb.h"
#include "api/ITransport.h"
#include "common/types/Result.h"

namespace camera_service::api {
    class Controller;
    class GrpcTransport final : public camera::CameraService::CallbackService, public ITransport {
    public:
        explicit GrpcTransport();
        void setController(Controller* controller) override;
        ~GrpcTransport() override;

        Result<void> start(const std::string& port) override;
        Result<void> stop() override;
        Result<void> runLoop() override;

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

    private:
        Controller* controller_;
        std::unique_ptr<grpc::Server> server_;
    };
}
