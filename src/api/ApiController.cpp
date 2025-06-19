#include "ApiController.h"

#include <future>

#include "core/Core.h"
#include "common/Logger/Logger.h"

namespace camera_service::api {
    ApiController::ApiController(std::unique_ptr<core::ICore> core, std::unique_ptr<IApiAdapter> api_adapter, const std::string& port)
        : core_(std::move(core)), running_(false), api_adapter_(std::move(api_adapter)), port_(port) {
        if (!core_) {
            throw ControllerException("Core cannot be null");
        }
        if (!api_adapter_) {
            throw ControllerException("API adapter cannot be null");
        }
        api_adapter_->setController(this);
    }

    ApiController::~ApiController() {
        if (running_) {
            stop();
        }
    }

    bool ApiController::startAsync() {
        LOG_INFO("Starting GRPC API Controller...");

        try {
            if (!core_->initialize()) {
                throw ControllerException("Core initialization failed");
            }

            if (!api_adapter_->start(port_)) {
                core_->shutdown();
                throw ControllerException("Failed to start API adapter");
            }

            running_ = true;

            std::thread([this]() {
                this->runLoop();
            }).detach();

            return true;

        } catch (const core::CoreException& e) {
            LOG_ERROR("Failed to start API Controller: {}", e.what());
            throw ControllerException(std::string("Core error during startup: ") + e.what());
        }
    }

    bool ApiController::stop() {
        if (!running_) {
            return true;
        }

        LOG_INFO("Stopping API Controller...");
        running_ = false;

        api_adapter_->stop();

        try {
            if (core_) {
                core_->shutdown();
            }
            return true;
        } catch (const core::CoreException& e) {
            LOG_ERROR("Error stopping core: {}", e.what());
            return false;
        }
    }

    bool ApiController::isRunning() const {
        return running_;
    }

    bool ApiController::setZoom(const double zoom_level) {
        if (!running_) {
            throw ControllerException("Controller is not running");
        }

        try {
            core_->setZoom(zoom_level);
            return true;
        } catch (const core::CoreException& e) {
            throw ControllerException(std::string("Core error during zoom operation: ") + e.what());
        }
    }

    double ApiController::getZoom() const {
        if (!running_) {
            throw ControllerException("Controller is not running");
        }

        try {
            return core_->getZoom();
        } catch (const core::CoreException& e) {
            throw ControllerException(std::string("Core error retrieving zoom: ") + e.what());
        }
    }

    bool ApiController::setFocus(const double focus_value) {
        if (!running_) {
            throw ControllerException("Controller is not running");
        }

        try {
            core_->setFocus(focus_value);
            return true;
        } catch (const core::CoreException& e) {
            throw ControllerException(std::string("Core error during focus operation: ") + e.what());
        }
    }

    double ApiController::getFocus() const {
        if (!running_) {
            throw ControllerException("Controller is not running");
        }

        try {
            return core_->getFocus();
        } catch (const core::CoreException& e) {
            throw ControllerException(std::string("Core error retrieving focus: ") + e.what());
        }
    }

    void ApiController::runLoop() {
        if (api_adapter_) {
            api_adapter_->runLoop();
        }
    }
}
