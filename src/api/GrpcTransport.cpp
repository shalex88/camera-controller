#include "GrpcTransport.h"

#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>
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
        LOG_DEBUG("Starting server...", server_address);
        //TODO: learn how to use health check
        grpc::EnableDefaultHealthCheckService(false);
        //TODO: disable in production
        grpc::reflection::InitProtoReflectionServerBuilderPlugin();

        grpc::ServerBuilder builder;
        builder.AddChannelArgument(GRPC_ARG_ALLOW_REUSEPORT, 0);
        int selected_port = 0;
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials(), &selected_port);
        builder.RegisterService(callback_handler_.get());

        // Temporarily suppress STDERR to hide gRPC internal error messages
        const int stderr_backup = dup(STDERR_FILENO);
        const int devnull = open("/dev/null", O_WRONLY);
        dup2(devnull, STDERR_FILENO);
        close(devnull);

        server_ = std::unique_ptr(builder.BuildAndStart());

        // Restore STDERR
        dup2(stderr_backup, STDERR_FILENO);
        close(stderr_backup);

        if (!server_ || selected_port == 0) {
            return Result<void>::error("Failed to start server on: " + server_address);
        }

        LOG_INFO("Server is listening on {}", server_address);
        return Result<void>::success();
    }

    Result<void> GrpcTransport::stop() {
        if (server_) {
            server_->Shutdown();
            server_.reset();
        }
        LOG_DEBUG("Server stopped");
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