#include "ApiController.h"

#include "api/RequestHandler.h"
#include "api/ITransport.h"
#include "common/Logger/Logger.h"

namespace camera_service::api {
    ApiController::ApiController(std::shared_ptr<IRequestHandler> request_handler, std::unique_ptr<ITransport> transport, const std::string& server_address, std::shared_ptr<LayerLogger> logger)
        : request_handler_(std::move(request_handler)), transport_(std::move(transport)), server_address_(server_address), running_(false), logger_(std::move(logger)) {
        if (!request_handler_) {
            throw std::invalid_argument("Request Handler cannot be null");
        }
        if (!transport_) {
            throw std::invalid_argument("Transport cannot be null");
        }
        if (server_address_.empty()) {
            throw std::invalid_argument("Server address cannot be empty");
        }
    }

    ApiController::~ApiController() {
        if (running_) {
            if (stop().isError()) {
                logger_->error("ApiController failed to stop gracefully");
            }
        }
    }

    Result<void> ApiController::startAsync() {
        logger_->info("Starting API ApiController...");

        if (const auto requst_handler_start_result = request_handler_->start(); requst_handler_start_result.isError()) {
            return Result<void>::error(logger_, "Failed to start: " + requst_handler_start_result.error());
        }

        if (const auto transport_result = transport_->start(server_address_); transport_result.isError()) {
            if (request_handler_->stop().isError()) {
                throw std::runtime_error("Request Handler is still running");
            }
            return Result<void>::error(transport_result.error());
        }

        running_ = true;

        service_thread_ = std::thread([this] {
            if (transport_->runLoop().isError()) {
                logger_->error("Transport run loop failed");
                running_ = false;
            } else {
                logger_->info("Transport run loop completed successfully");
            }
        });

        return Result<void>::success();
    }

    Result<void> ApiController::stop() {
        if (!isRunning()) {
            return Result<void>::success();
        }

        logger_->info("Stopping API ApiController...");
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

    bool ApiController::isRunning() const {
        return running_;
    }
}
