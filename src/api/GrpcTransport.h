#pragma once

#include "api/ITransport.h"
#include "api/proto/camera_service.grpc.pb.h" //TODO: can move to implementation file?
#include "common/types/Result.h"

namespace camera_service::common {
    class LayerLogger;
}

namespace camera_service::api {
    class IRequestHandler;
    class GrpcCallbackHandler;

    class GrpcTransport final : public ITransport {
    public:
        explicit GrpcTransport(std::shared_ptr<IRequestHandler> request_handler,
                              std::shared_ptr<common::LayerLogger> logger);
        ~GrpcTransport() override;

        Result<void> start(const std::string& server_address) override;
        Result<void> stop() override;
        Result<void> runLoop() override;

    private:
        std::unique_ptr<GrpcCallbackHandler> callback_handler_;
        std::unique_ptr<grpc::Server> server_;
        std::shared_ptr<common::LayerLogger> logger_;
    };
}
