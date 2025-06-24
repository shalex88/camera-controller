#include "GrpcTransport.h"

#include <iostream>
#include <future>
#include <grpcpp/grpcpp.h>

#include "Controller.h"
#include "common/Logger/Logger.h"

namespace camera_service::api {
    GrpcTransport::GrpcTransport() : controller_(nullptr) {
    }

    GrpcTransport::~GrpcTransport() {
        stop();
    }

    void GrpcTransport::setController(Controller* controller) {
        if (!controller) {
            throw std::invalid_argument("Controller cannot be null");
        }
        controller_ = controller;
    }

    Result<void> GrpcTransport::start(const std::string& port) {
        const std::string server_address("localhost:" + port);

        grpc::EnableDefaultHealthCheckService(true);
        grpc::ServerBuilder builder;
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        builder.RegisterService(this);

        server_ = std::unique_ptr<grpc::Server>(builder.BuildAndStart());
        if (!server_) {
            LOG_ERROR("Failed to start gRPC server");
            return Result<void>::error("Failed to start gRPC server");
        }

        LOG_INFO("Service is listening on {}", server_address);
        return Result<void>::success();
    }

    Result<void> GrpcTransport::stop() {
        if (server_) {
            server_->Shutdown();
            server_.reset();
        }
        return Result<void>::success();
    }

    Result<void> GrpcTransport::runLoop() {
        if (!server_) {
            return Result<void>::error("Server not initialized");
        }
        server_->Wait();
        return Result<void>::success();
    }

    template<typename RequestType, typename ResponseType, typename ProcessFunc>
    grpc::ServerUnaryReactor* handleGrpcRequest(
        grpc::CallbackServerContext* context,
        const RequestType* request,
        ResponseType* response,
        ProcessFunc processFunction) {
        auto reactor = context->DefaultReactor();
        auto deadline = grpc::Timespec2Timepoint(context->raw_deadline());

        // Launch the processing task asynchronously
        std::future<grpc::Status> future = std::async(std::launch::async, [request, response, processFunction]() {
            try {
                auto result = processFunction(request, response);
                if (result.isError()) {
                    return grpc::Status(grpc::StatusCode::INTERNAL, result.error());
                }
                return grpc::Status::OK;
            } catch (const std::exception& e) {
                return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
            }
        });

        // Calculate remaining time before the deadline
        auto now = std::chrono::system_clock::now();
        auto remaining_time = (deadline > now) ? (deadline - now) : std::chrono::seconds(0);

        // Wait for the processing to complete or timeout
        if (future.wait_for(remaining_time) == std::future_status::timeout) {
            LOG_ERROR("Request exceeded deadline during processing.");
            reactor->Finish(grpc::Status(grpc::StatusCode::DEADLINE_EXCEEDED, "Processing exceeded deadline"));
        } else {
            reactor->Finish(future.get());
        }

        return reactor;
    }

    grpc::ServerUnaryReactor* GrpcTransport::SetZoom(
        grpc::CallbackServerContext* context,
        const camera::SetZoomRequest* request,
        camera::SetZoomResponse* response) {
        return handleGrpcRequest(context, request, response,
            [this](const camera::SetZoomRequest* req, camera::SetZoomResponse* resp) {
                LOG_INFO("Request: SetZoom to {}", req->zoom());
                return controller_->setZoom(req->zoom());
            });
    }

    grpc::ServerUnaryReactor* GrpcTransport::SetFocus(
        grpc::CallbackServerContext* context,
        const camera::SetFocusRequest* request,
        camera::SetFocusResponse* response) {
        return handleGrpcRequest(context, request, response,
            [this](const camera::SetFocusRequest* req, camera::SetFocusResponse* resp) {
                LOG_INFO("Request: SetFocus to {}", req->focus());
                return controller_->setFocus(req->focus());
            });
    }

    grpc::ServerUnaryReactor* GrpcTransport::GetZoom(
        grpc::CallbackServerContext* context,
        const camera::GetZoomRequest* request,
        camera::GetZoomResponse* response) {
        return handleGrpcRequest(context, request, response,
            [this](const camera::GetZoomRequest* req, camera::GetZoomResponse* resp) {
                LOG_INFO("Request: GetZoom");
                auto result = controller_->getZoom();
                if (result.isSuccess()) {
                    resp->set_zoom(result.value());
                    return Result<void>::success();
                }
                return Result<void>::error(result.error());
            });
    }

    grpc::ServerUnaryReactor* GrpcTransport::GetFocus(
        grpc::CallbackServerContext* context,
        const camera::GetFocusRequest* request,
        camera::GetFocusResponse* response) {
        return handleGrpcRequest(context, request, response,
            [this](const camera::GetFocusRequest* req, camera::GetFocusResponse* resp) {
                LOG_INFO("Request: GetFocus");
                auto result = controller_->getFocus();
                if (result.isSuccess()) {
                    resp->set_focus(result.value());
                    return Result<void>::success();
                }
                return Result<void>::error(result.error());
            });
    }
};
