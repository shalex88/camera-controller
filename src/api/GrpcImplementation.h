#pragma once

#include "camera_service.pb.h"
#include "camera_service.grpc.pb.h"

namespace camera_service::api {
    class GrpcImplementation final : public camera::CameraService::CallbackService {
    public:
        explicit GrpcImplementation(GrpcController* controller);

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
        GrpcController* controller_;
    };
}
