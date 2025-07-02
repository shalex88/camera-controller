#pragma once
#include <memory>
#include <string>
#include <thread>
#include <atomic>

#include "common/types/Result.h"
#include "api/RequestHandler.h"
#include "api/ITransport.h"

namespace camera_service::api {
    class Controller final {
    public:
        explicit Controller(std::shared_ptr<IRequestHandler> request_handler, std::unique_ptr<ITransport> transport, const std::string& server_address);
        ~Controller();

        Result<void> startAsync();
        Result<void> stop();
        bool isRunning() const;

    private:
        std::shared_ptr<IRequestHandler> request_handler_;
        std::unique_ptr<ITransport> transport_;
        std::string server_address_;
        std::atomic<bool> running_;
        std::thread service_thread_;
    };
}
