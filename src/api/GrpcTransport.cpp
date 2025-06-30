#include "GrpcTransport.h"

#include <grpcpp/grpcpp.h>

#include "api/RequestHandler.h"
#include "api/GrpcCallbackHandler.h"
#include "common/Logger/Logger.h"

namespace camera_service::api {
    GrpcTransport::GrpcTransport(std::shared_ptr<RequestHandler> request_handler) {
        if (!request_handler) {
            throw std::invalid_argument("Request Handler cannot be null");
        }
        callback_handler_ = std::make_unique<GrpcCallbackHandler>(std::move(request_handler));
    }

    GrpcTransport::~GrpcTransport() {
        stop();
    }

    Result<void> GrpcTransport::start(const std::string& port) {
        const std::string server_address("localhost:" + port);

        grpc::EnableDefaultHealthCheckService(true);
        grpc::ServerBuilder builder;
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        builder.RegisterService(callback_handler_.get());

        server_ = std::unique_ptr(builder.BuildAndStart());
        if (!server_) {
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
}