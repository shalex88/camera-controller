#pragma once
#include <memory>
#include <string>
#include <thread>
#include <atomic>

#include "common/types/Result.h"
#include "api/RequestHandler.h"
#include "api/ITransport.h"

namespace camera_service::api {
    class ApiController final {
    public:
        explicit ApiController(std::shared_ptr<IRequestHandler> request_handler, std::unique_ptr<ITransport> transport, const std::string& server_address, std::shared_ptr<LayerLogger> logger);
        ~ApiController();

        Result<void> startAsync();
        Result<void> stop();
        bool isRunning() const;

    private:
        std::shared_ptr<IRequestHandler> request_handler_;
        std::unique_ptr<ITransport> transport_;
        std::string server_address_;
        std::atomic<bool> running_;
        std::thread service_thread_;
        std::shared_ptr<LayerLogger> logger_;
    };
}
