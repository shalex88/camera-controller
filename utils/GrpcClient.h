#pragma once
#include <grpcpp/grpcpp.h>

#include "api/proto/camera_service.grpc.pb.h"
#include "api/proto/camera_service.pb.h"
#include "common/types/CameraTypes.h"
#include "common/types/Result.h"

using namespace service;
using namespace std::chrono_literals;

#define NFOV_CAMERA_LOCK_TIMEOUT_MS 400ms

class GrpcClient {
public:
    explicit GrpcClient(const std::shared_ptr<grpc::Channel>& channel)
        : stub_(camera::v1::CameraService::NewStub(channel)) {}

    Result<void> setZoom(const common::types::zoom zoom_value, const std::chrono::milliseconds timeout = NFOV_CAMERA_LOCK_TIMEOUT_MS) const {
        camera::v1::SetZoomRequest request;
        camera::v1::SetZoomResponse response;
        grpc::ClientContext context;

        request.set_zoom(zoom_value);
        context.set_deadline(std::chrono::system_clock::now() + timeout);

        if (const grpc::Status status = stub_->SetZoom(&context, request, &response); !status.ok()) {
            return Result<void>::error(status.error_message());
        }
        return Result<void>::success();
    }

    Result<common::types::zoom> getZoom(const std::chrono::milliseconds timeout = NFOV_CAMERA_LOCK_TIMEOUT_MS) const {
        const google::protobuf::Empty request;
        camera::v1::GetZoomResponse response;
        grpc::ClientContext context;

        context.set_deadline(std::chrono::system_clock::now() + timeout);

        if (const grpc::Status status = stub_->GetZoom(&context, request, &response); !status.ok()) {
            return Result<common::types::zoom>::error(status.error_message());
        }
        return Result<common::types::zoom>::success(response.zoom());
    }

    Result<void> setFocus(const common::types::focus focus_value, const std::chrono::milliseconds timeout = NFOV_CAMERA_LOCK_TIMEOUT_MS) const {
        camera::v1::SetFocusRequest request;
        camera::v1::SetFocusResponse response;
        grpc::ClientContext context;

        request.set_focus(focus_value);
        context.set_deadline(std::chrono::system_clock::now() + timeout);

        if (const grpc::Status status = stub_->SetFocus(&context, request, &response); !status.ok()) {
            return Result<void>::error(status.error_message());
        }
        return Result<void>::success();
    }

    Result<common::types::focus> getFocus(const std::chrono::milliseconds timeout = NFOV_CAMERA_LOCK_TIMEOUT_MS) const {
        const google::protobuf::Empty request;
        camera::v1::GetFocusResponse response;
        grpc::ClientContext context;

        context.set_deadline(std::chrono::system_clock::now() + timeout);

        if (const grpc::Status status = stub_->GetFocus(&context, request, &response); !status.ok()) {
            return Result<common::types::focus>::error(status.error_message());
        }
        return Result<common::types::focus>::success(response.focus());
    }

    Result<void> setAutoFocus(const auto enable, const std::chrono::milliseconds timeout = NFOV_CAMERA_LOCK_TIMEOUT_MS) const {
        camera::v1::SetAutoFocusRequest request;
        google::protobuf::Empty response;
        grpc::ClientContext context;

        request.set_enable(enable);
        context.set_deadline(std::chrono::system_clock::now() + timeout);

        if (const grpc::Status status = stub_->SetAutoFocus(&context, request, &response); !status.ok()) {
            return Result<void>::error(status.error_message());
        }
        return Result<void>::success();
    }

private:
    std::unique_ptr<camera::v1::CameraService::Stub> stub_;
};