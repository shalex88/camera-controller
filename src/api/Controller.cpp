#include "Controller.h"

#include <future>

#include "core/Core.h"
#include "common/Logger/Logger.h"

namespace camera_service::api {
    Controller::Controller(std::unique_ptr<core::ICore> core, std::unique_ptr<ITransport> transport, const std::string& port)
        : transport_(std::move(transport)), core_(std::move(core)), running_(false), port_(port) {
        if (!core_) {
            throw ControllerException("Core cannot be null");
        }
        if (!transport_) {
            throw ControllerException("Controller implementation cannot be null");
        }
        if (port_.empty()) {
            throw ControllerException("Port cannot be empty");
        }
        transport_->setController(this);
    }

    Controller::~Controller() {
        if (running_) {
            stop();
        }
    }

    bool Controller::startAsync() {
        LOG_INFO("Starting GRPC API Controller...");

        try {
            if (!core_->initialize()) {
                throw ControllerException("Core initialization failed");
            }

            if (!transport_->start(port_)) {
                core_->shutdown();
                throw ControllerException("Failed to start Controller implementation");
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

    bool Controller::stop() {
        if (!running_) {
            return true;
        }

        LOG_INFO("Stopping API Controller...");
        running_ = false;

        transport_->stop();

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

    bool Controller::isRunning() const {
        return running_;
    }

    void Controller::runLoop() const {
        if (transport_) {
            transport_->runLoop();
        }
    }

    void Controller::setZoom(const types::zoom zoom_level) const {
        if (!running_) {
            throw ControllerException("Controller is not running");
        }

        try {
            core_->setZoom(zoom_level);
        } catch (const core::CoreException& e) {
            throw ControllerException(std::string("Core error during zoom operation: ") + e.what());
        }
    }

    types::zoom Controller::getZoom() const {
        if (!running_) {
            throw ControllerException("Controller is not running");
        }

        try {
            return core_->getZoom();
        } catch (const core::CoreException& e) {
            throw ControllerException(std::string("Core error retrieving zoom: ") + e.what());
        }
    }

    void Controller::setFocus(const types::focus focus_value) const {
        if (!running_) {
            throw ControllerException("Controller is not running");
        }

        try {
            core_->setFocus(focus_value);
        } catch (const core::CoreException& e) {
            throw ControllerException(std::string("Core error during focus operation: ") + e.what());
        }
    }

    types::focus Controller::getFocus() const {
        if (!running_) {
            throw ControllerException("Controller is not running");
        }

        try {
            return core_->getFocus();
        } catch (const core::CoreException& e) {
            throw ControllerException(std::string("Core error retrieving focus: ") + e.what());
        }
    }
}
