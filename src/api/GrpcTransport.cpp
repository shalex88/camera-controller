#include "GrpcTransport.h"

#include <grpcpp/grpcpp.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>

#include "api/GrpcCallbackHandler.h"
#include "api/IRequestHandler.h"
#include "common/logger/Logger.h"

namespace camera_service::api {
    GrpcTransport::GrpcTransport(std::shared_ptr<IRequestHandler> request_handler) {
        if (!request_handler) {
            throw std::invalid_argument("Request Handler cannot be null");
        }
        callback_handler_ = std::make_unique<GrpcCallbackHandler>(std::move(request_handler));
    }

    GrpcTransport::~GrpcTransport() {
        if (stop().isError()) {
            LOG_ERROR("Failed to stop the gRPC server");
        }
    }

    Result<void> GrpcTransport::start(const std::string& server_address) {
        //TODO: learn how to use health check
        grpc::EnableDefaultHealthCheckService(false);
        //TODO: disable in production
        grpc::reflection::InitProtoReflectionServerBuilderPlugin();

        grpc::ServerBuilder builder;
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        builder.RegisterService(callback_handler_.get());

        server_ = std::unique_ptr(builder.BuildAndStart());
        if (!server_) {
            return Result<void>::error("Failed to start the gRPC server");
        }

        LOG_INFO("Listening on {}", server_address);
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