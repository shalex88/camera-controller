#include "GrpcCallbackHandler.h"

#include <future>
#include <grpcpp/grpcpp.h>

#include "common/Logger/Logger.h"

namespace camera_service::api {
    GrpcCallbackHandler::GrpcCallbackHandler(std::shared_ptr<IRequestHandler> request_handler)
        : request_handler_(request_handler) {
        if (!request_handler_) {
            throw std::invalid_argument("Request Handler cannot be null");
        }
    }

    template<typename RequestType, typename ResponseType, typename ProcessFunc>
    grpc::ServerUnaryReactor* handleGrpcRequest(
        grpc::CallbackServerContext* context,
        const RequestType* request,
        ResponseType* response,
        ProcessFunc process_function) {
        const auto reactor = context->DefaultReactor();
        const auto deadline = grpc::Timespec2Timepoint(context->raw_deadline());

        // Calculate the remaining time before the deadline
        const auto now = std::chrono::system_clock::now();
        const auto remaining_time = deadline > now ? deadline - now : std::chrono::seconds(0);

        // Launch the processing task asynchronously
        std::future<grpc::Status> future = std::async(std::launch::async, [request, response, process_function] {
            if (auto result = process_function(request, response); result.isError()) {
                return grpc::Status(grpc::StatusCode::INTERNAL, result.error());
            }
            return grpc::Status::OK;
        });

        //TODO: handle task execution abort if deadline exceeds, think of a way to cancel and undo the task

        // Wait for the processing to complete or timeout
        if (future.wait_for(remaining_time) == std::future_status::timeout) {
            LOG_ERROR("Request exceeded deadline during processing.");
            reactor->Finish(grpc::Status(grpc::StatusCode::DEADLINE_EXCEEDED, "Processing exceeded deadline"));
        } else {
            reactor->Finish(future.get());
        }

        return reactor;
    }

    grpc::ServerUnaryReactor* GrpcCallbackHandler::SetZoom(
        grpc::CallbackServerContext* context,
        const camera::SetZoomRequest* request,
        camera::SetZoomResponse* response) {
        return handleGrpcRequest(context, request, response,
            [this](const camera::SetZoomRequest* req, camera::SetZoomResponse* resp) {
                return request_handler_->setZoom(req->zoom());
            });
    }

    grpc::ServerUnaryReactor* GrpcCallbackHandler::SetFocus(
        grpc::CallbackServerContext* context,
        const camera::SetFocusRequest* request,
        camera::SetFocusResponse* response) {
        return handleGrpcRequest(context, request, response,
            [this](const camera::SetFocusRequest* req, camera::SetFocusResponse* resp) {
                return request_handler_->setFocus(req->focus());
            });
    }

    grpc::ServerUnaryReactor* GrpcCallbackHandler::GetZoom(
        grpc::CallbackServerContext* context,
        const camera::GetZoomRequest* request,
        camera::GetZoomResponse* response) {
        return handleGrpcRequest(context, request, response,
            [this](const camera::GetZoomRequest* req, camera::GetZoomResponse* resp) {
                const auto result = request_handler_->getZoom();
                if (result.isSuccess()) {
                    resp->set_zoom(result.value());
                    return Result<void>::success();
                }
                return Result<void>::error(result.error());
            });
    }

    grpc::ServerUnaryReactor* GrpcCallbackHandler::GetFocus(
        grpc::CallbackServerContext* context,
        const camera::GetFocusRequest* request,
        camera::GetFocusResponse* response) {
        return handleGrpcRequest(context, request, response,
            [this](const camera::GetFocusRequest* req, camera::GetFocusResponse* resp) {
                const auto result = request_handler_->getFocus();
                if (result.isSuccess()) {
                    resp->set_focus(result.value());
                    return Result<void>::success();
                }
                return Result<void>::error(result.error());
            });
    }

    grpc::ServerUnaryReactor* GrpcCallbackHandler::GetInfo(
        grpc::CallbackServerContext* context,
        const camera::GetInfoRequest* request,
        camera::GetInfoResponse* response) {
        return handleGrpcRequest(context, request, response,
            [this](const camera::GetInfoRequest* req, camera::GetInfoResponse* resp) {
                const auto result = request_handler_->getInfo();
                if (result.isSuccess()) {
                    resp->set_info(result.value());
                    return Result<void>::success();
                }
                return Result<void>::error(result.error());
            });
    }

    grpc::ServerUnaryReactor* GrpcCallbackHandler::SetMinZoom(
        grpc::CallbackServerContext* context,
        const camera::SetMinZoomRequest* request,
        camera::SetMinZoomResponse* response) {
        return handleGrpcRequest(context, request, response,
            [this](const camera::SetMinZoomRequest* req, camera::SetMinZoomResponse* resp) {
                const auto result = request_handler_->setMinZoom();
                if (result.isSuccess()) {
                    return Result<void>::success();
                }
                return Result<void>::error(result.error());
            });
    }

    grpc::ServerUnaryReactor* GrpcCallbackHandler::SetMaxZoom(
        grpc::CallbackServerContext* context,
        const camera::SetMaxZoomRequest* request,
        camera::SetMaxZoomResponse* response) {
        return handleGrpcRequest(context, request, response,
            [this](const camera::SetMaxZoomRequest* req, camera::SetMaxZoomResponse* resp) {
                const auto result = request_handler_->setMaxZoom();
                if (result.isSuccess()) {
                    return Result<void>::success();
                }
                return Result<void>::error(result.error());
            });
    }
}
