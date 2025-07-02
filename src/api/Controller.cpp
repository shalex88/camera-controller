#include "Controller.h"

#include "api/RequestHandler.h"
#include "api/ITransport.h"
#include "common/Logger/Logger.h"

namespace camera_service::api {
    Controller::Controller(std::shared_ptr<IRequestHandler> request_handler, std::unique_ptr<ITransport> transport, const std::string& server_address)
        : request_handler_(std::move(request_handler)), transport_(std::move(transport)), server_address_(server_address), running_(false) {
        if (!request_handler_) {
            throw std::invalid_argument("Request Handler cannot be null");
        }
        if (!transport_) {
            throw std::invalid_argument("Transport cannot be null");
        }
        if (server_address_.empty()) {
            throw std::invalid_argument("server_address cannot be empty");
        }
    }

    Controller::~Controller() {
        if (isRunning()) {
            stop();
        }
    }

    Result<void> Controller::startAsync() {
        LOG_INFO("Starting API Controller...");

        if (const auto requst_handler_start_result = request_handler_->start(); requst_handler_start_result.isError()) {
            return Result<void>::error("Failed to start request handler: " + requst_handler_start_result.error());
        }

        if (const auto transport_result = transport_->start(server_address_); transport_result.isError()) {
            request_handler_->stop();
            return Result<void>::error("Failed to start transport: " + transport_result.error());
        }

        running_ = true;

        service_thread_ = std::thread([this]() {
            transport_->runLoop();
        });

        return Result<void>::success();
    }

    Result<void> Controller::stop() {
        if (!isRunning()) {
            return Result<void>::success();
        }

        LOG_INFO("Stopping API Controller...");
        running_ = false;

        if (const auto transport_result = transport_->stop(); transport_result.isError()) {
            service_thread_.join();
            return Result<void>::error("Error stopping transport: " + transport_result.error());
        }

        if (service_thread_.joinable()) {
            service_thread_.join();
        }

        if (const auto stop_result = request_handler_->stop(); stop_result.isError()) {
            return Result<void>::error("Failed to stop request handler: " + stop_result.error());
        }

        return Result<void>::success();
    }

    bool Controller::isRunning() const {
        return running_;
    }
}
