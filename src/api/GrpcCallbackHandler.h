#pragma once

#include "api/proto/camera_service.pb.h"
#include "api/proto/camera_service.grpc.pb.h"
#include "api/IRequestHandler.h"

namespace camera_service::api {
    class GrpcCallbackHandler final : public camera::CameraService::CallbackService {
    public:
        explicit GrpcCallbackHandler(std::shared_ptr<IRequestHandler> request_handler);

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

        grpc::ServerUnaryReactor* GetInfo(
            grpc::CallbackServerContext* context,
            const camera::GetInfoRequest* request,
            camera::GetInfoResponse* response) override;

        grpc::ServerUnaryReactor* SetMinZoom(
            grpc::CallbackServerContext* context,
            const camera::SetMinZoomRequest* request,
            camera::SetMinZoomResponse* response) override;

        grpc::ServerUnaryReactor* SetMaxZoom(
            grpc::CallbackServerContext* context,
            const camera::SetMaxZoomRequest* request,
            camera::SetMaxZoomResponse* response) override;

    private:
        std::shared_ptr<IRequestHandler> request_handler_;
    };
}
