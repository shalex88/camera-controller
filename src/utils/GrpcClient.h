#pragma once
#include <grpcpp/grpcpp.h>

#include "api/proto/camera_service.grpc.pb.h"
#include "api/proto/camera_service.pb.h"
#include "common/types/Result.h"

using namespace camera_service;
using namespace std::chrono_literals;

class GrpcClient {
public:
    explicit GrpcClient(const std::shared_ptr<grpc::Channel>& channel)
        : stub_(camera::CameraService::NewStub(channel)) {}

    Result<void> setZoom(const types::zoom zoom_value, const std::chrono::milliseconds timeout = 300ms) const {
        camera::SetZoomRequest request;
        camera::SetZoomResponse response;
        grpc::ClientContext context;

        request.set_zoom(zoom_value);
        context.set_deadline(std::chrono::system_clock::now() + timeout);

        if (const grpc::Status status = stub_->SetZoom(&context, request, &response); !status.ok()) {
            return Result<void>::error(status.error_message());
        }
        return Result<void>::success();
    }

    Result<types::zoom> getZoom(const std::chrono::milliseconds timeout = 300ms) const {
        const camera::GetZoomRequest request;
        camera::GetZoomResponse response;
        grpc::ClientContext context;

        context.set_deadline(std::chrono::system_clock::now() + timeout);

        if (const grpc::Status status = stub_->GetZoom(&context, request, &response); !status.ok()) {
            return Result<types::zoom>::error(status.error_message());
        }
        return Result<types::zoom>::success(response.zoom());
    }

    Result<void> setFocus(const types::focus focus_value, const std::chrono::milliseconds timeout = 300ms) const {
        camera::SetFocusRequest request;
        camera::SetFocusResponse response;
        grpc::ClientContext context;

        request.set_focus(focus_value);
        context.set_deadline(std::chrono::system_clock::now() + timeout);

        if (const grpc::Status status = stub_->SetFocus(&context, request, &response); !status.ok()) {
            return Result<void>::error(status.error_message());
        }
        return Result<void>::success();
    }

    Result<types::focus> getFocus(const std::chrono::milliseconds timeout = 300ms) const {
        const camera::GetFocusRequest request;
        camera::GetFocusResponse response;
        grpc::ClientContext context;

        context.set_deadline(std::chrono::system_clock::now() + timeout);

        if (const grpc::Status status = stub_->GetFocus(&context, request, &response); !status.ok()) {
            return Result<types::focus>::error(status.error_message());
        }
        return Result<types::focus>::success(response.focus());
    }

private:
    std::unique_ptr<camera::CameraService::Stub> stub_;
};