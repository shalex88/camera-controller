#include "GrpcTransport.h"

#include <future>
#include <grpcpp/grpcpp.h>

#include "api/Controller.h"
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

        server_ = std::unique_ptr(builder.BuildAndStart());
        if (!server_) {
            LOG_ERROR("Failed to start the gRPC server");
            return Result<void>::error("Failed to start the gRPC server");
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
            return Result<void>::error("Server isn't initialized");
        }
        server_->Wait();
        return Result<void>::success();
    }

    template<typename RequestType, typename ResponseType, typename ProcessFunc>
    grpc::ServerUnaryReactor* handleGrpcRequest(
        grpc::CallbackServerContext* context,
        const RequestType* request,
        ResponseType* response,
        ProcessFunc process_function) {
        const auto reactor = context->DefaultReactor();
        const auto deadline = grpc::Timespec2Timepoint(context->raw_deadline());

        // Launch the processing task asynchronously
        std::future<grpc::Status> future = std::async(std::launch::async, [request, response, process_function]() {
            try {
                if (auto result = process_function(request, response); result.isError()) {
                    return grpc::Status(grpc::StatusCode::INTERNAL, result.error());
                }
                return grpc::Status::OK;
            } catch (const std::exception& e) {
                return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
            }
        });

        // Calculate the remaining time before the deadline
        const auto now = std::chrono::system_clock::now();
        const auto remaining_time = deadline > now ? deadline - now : std::chrono::seconds(0);

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
                const auto result = controller_->getZoom();
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
                const auto result = controller_->getFocus();
                if (result.isSuccess()) {
                    resp->set_focus(result.value());
                    return Result<void>::success();
                }
                return Result<void>::error(result.error());
            });
    }
}