#include "Controller.h"

#include <future>

#include "core/Core.h"
#include "common/Logger/Logger.h"

namespace camera_service::api {
    Controller::Controller(std::unique_ptr<core::ICore> core, std::unique_ptr<ITransport> transport, const std::string& port)
        : transport_(std::move(transport)), core_(std::move(core)), running_(false), port_(port) {
        if (!core_) {
            throw std::invalid_argument("Core cannot be null");
        }
        if (!transport_) {
            throw std::invalid_argument("Controller implementation cannot be null");
        }
        if (port_.empty()) {
            throw std::invalid_argument("Port cannot be empty");
        }
        transport_->setController(this);
    }

    Controller::~Controller() {
        if (running_) {
            stop();
        }
    }

    Result<void> Controller::startAsync() {
        LOG_INFO("Starting GRPC API Controller...");

        if (const auto init_result = core_->initialize(); init_result.isError()) {
            return Result<void>::error("Core initialization failed: " + init_result.error());
        }

        if (const auto result = transport_->start(port_); result.isError()) {
            core_->shutdown();
            return Result<void>::error("Failed to start Controller implementation: " + result.error());
        }

        running_ = true;

        std::thread([this]() {
            this->runLoop();
        }).detach();

        return Result<void>::success();
    }

    Result<void> Controller::stop() {
        if (!running_) {
            return Result<void>::success();
        }

        LOG_INFO("Stopping API Controller...");
        running_ = false;

        if (const auto stop_result = transport_->stop(); stop_result.isError()) {
            LOG_ERROR("Error stopping transport: {}", stop_result.error());
        }

        if (core_) {
            if (const auto shutdown_result = core_->shutdown(); shutdown_result.isError()) {
                LOG_ERROR("Error stopping core: {}", shutdown_result.error());
                return Result<void>::error("Failed to shut down core: " + shutdown_result.error());
            }
        }
        return Result<void>::success();
    }

    bool Controller::isRunning() const {
        return running_;
    }

    Result<void> Controller::runLoop() const {
        if (transport_) {
            if (const auto loop_result = transport_->runLoop(); loop_result.isError()) {
                return Result<void>::error("Transport loop failed: " + loop_result.error());
            }
            return Result<void>::success();
        }
        return Result<void>::error("Transport isn't initialized");
    }

    Result<void> Controller::setZoom(const types::zoom zoom_level) const {
        if (!running_) {
            return Result<void>::error("Controller is not running");
        }

        return core_->setZoom(zoom_level);
    }

    Result<types::zoom> Controller::getZoom() const {
        if (!running_) {
            return Result<types::zoom>::error("Controller is not running");
        }

        return core_->getZoom();
    }

    Result<void> Controller::setFocus(const types::focus focus_value) const {
        if (!running_) {
            return Result<void>::error("Controller is not running");
        }

        return core_->setFocus(focus_value);
    }

    Result<types::focus> Controller::getFocus() const {
        if (!running_) {
            return Result<types::focus>::error("Controller is not running");
        }

        return core_->getFocus();
    }
}
