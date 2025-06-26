#pragma once
#include <memory>
#include <string>
#include <thread>
#include <atomic>

#include "common/types/Result.h"

namespace camera_service::api {
    class RequestHandler;
    class ITransport;

    class Controller final {
    public:
        explicit Controller(std::shared_ptr<RequestHandler> controller, std::unique_ptr<ITransport> transport, const std::string& port);
        ~Controller();

        Result<void> startAsync();
        Result<void> stop();
        bool isRunning() const;

    private:
        std::shared_ptr<RequestHandler> request_handler_;
        std::unique_ptr<ITransport> transport_;
        std::string port_;
        std::atomic<bool> running_;
        std::thread service_thread_;
    };
}
