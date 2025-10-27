#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <thread>

#include "common/types/Result.h"

namespace camera_service::api {
    class IRequestHandler;
    class ITransport;

    class ApiController final {
    public:
        explicit ApiController(std::shared_ptr<IRequestHandler> request_handler, std::unique_ptr<ITransport> transport, std::string  server_address, std::shared_ptr<common::LayerLogger> logger);
        ~ApiController();

        Result<void> startAsync();
        Result<void> stop();
        bool isRunning() const;

    private:
        std::shared_ptr<IRequestHandler> request_handler_;
        std::unique_ptr<ITransport> transport_;
        std::string server_address_;
        std::atomic<bool> running_;
        std::jthread service_thread_;
        std::shared_ptr<common::LayerLogger> logger_;
    };
}
