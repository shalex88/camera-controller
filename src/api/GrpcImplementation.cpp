#include "GrpcController.h"

#include "GrpcImplementation.h"

#include <iostream>
#include <future>

#include "common/Logger/Logger.h"

namespace camera_service::api {
    template<typename RequestType, typename ResponseType, typename ProcessFunc>
    grpc::ServerUnaryReactor* handleGrpcRequest(
        grpc::CallbackServerContext* context,
        const RequestType* request,
        ResponseType* response,
        ProcessFunc processFunction) {
        const auto reactor = context->DefaultReactor();
        const auto deadline = grpc::Timespec2Timepoint(context->raw_deadline());

        // Launch the processing task asynchronously
        const std::future<void> future = std::async(std::launch::async, [request, response, processFunction]() {
            processFunction(request, response);
        });

        // Calculate remaining time before the deadline
        const auto now = std::chrono::system_clock::now();
        const auto remaining_time = (deadline > now) ? (deadline - now) : std::chrono::seconds(0);

        // Wait for the processing to complete or timeout
        if (future.wait_for(remaining_time) == std::future_status::timeout) {
            std::cerr << "Request exceeded deadline during processing." << std::endl;
            reactor->Finish(grpc::Status(grpc::StatusCode::DEADLINE_EXCEEDED, "Processing exceeded deadline"));
        } else {
            reactor->Finish(grpc::Status::OK);
        }

        return reactor;
    }

    GrpcImplementation::GrpcImplementation(GrpcController* controller) : controller_(controller) {
    }

    grpc::ServerUnaryReactor* GrpcImplementation::SetZoom(
        grpc::CallbackServerContext* context,
        const camera::SetZoomRequest* request,
        camera::SetZoomResponse* response) {
        return handleGrpcRequest(context, request, response,
                                 [this](const camera::SetZoomRequest* req, camera::SetZoomResponse* resp) {
                                     LOG_INFO("Received: SetZoom to {}", req->zoom());
                                     controller_->setZoom(req->zoom());
                                 });
    }

    grpc::ServerUnaryReactor* GrpcImplementation::SetFocus(
        grpc::CallbackServerContext* context,
        const camera::SetFocusRequest* request,
        camera::SetFocusResponse* response) {
        return handleGrpcRequest(context, request, response,
                                 [this](const camera::SetFocusRequest* req, camera::SetFocusResponse* resp) {
                                     LOG_INFO("Received: SetFocus to {}", req->focus());
                                     controller_->setFocus(req->focus());
                                 });
    }

    grpc::ServerUnaryReactor* GrpcImplementation::GetZoom(
        grpc::CallbackServerContext* context,
        const camera::GetZoomRequest* request,
        camera::GetZoomResponse* response) {
        return handleGrpcRequest(context, request, response,
                                 [this](const camera::GetZoomRequest* req, camera::GetZoomResponse* resp) {
                                     LOG_INFO("Received: GetZoom");
                                     resp->set_zoom(controller_->getZoom());
                                 });
    }

    grpc::ServerUnaryReactor* GrpcImplementation::GetFocus(
        grpc::CallbackServerContext* context,
        const camera::GetFocusRequest* request,
        camera::GetFocusResponse* response) {
        return handleGrpcRequest(context, request, response,
                                 [this](const camera::GetFocusRequest* req, camera::GetFocusResponse* resp) {
                                     LOG_INFO("Received: GetFocus");
                                     resp->set_focus(controller_->getFocus());
                                 });
    }
};
