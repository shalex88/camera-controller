#pragma once

#include "api/proto/camera_service.grpc.pb.h"
#include "api/ITransport.h"
#include "api/IRequestHandler.h"
#include "api/GrpcCallbackHandler.h"
#include "common/types/Result.h"

namespace camera_service::api {
    class GrpcTransport final : public ITransport {
    public:
        explicit GrpcTransport(std::shared_ptr<IRequestHandler> request_handler);
        ~GrpcTransport() override;

        Result<void> start(const std::string& server_address) override;
        Result<void> stop() override;
        Result<void> runLoop() override;

    private:
        std::unique_ptr<GrpcCallbackHandler> callback_handler_;
        std::unique_ptr<grpc::Server> server_;
    };
}
