#include "GrpcController.h"

#include <iostream>
#include <future>
#include <grpcpp/grpcpp.h>

#include "core/Core.h"
#include "common/Logger/Logger.h"
#include "GrpcImplementation.h"

namespace camera_service::api {
    GrpcController::GrpcController(std::unique_ptr<core::ICore> core)
        : core_(std::move(core)), running_(false) {
        if (!core_) {
            throw ControllerException("Cannot initialize CameraController with null Core");
        }
    }

    GrpcController::~GrpcController() {
        if (running_) {
            stop();
        }
    }

    bool GrpcController::startAsync() {
        LOG_INFO("Starting GRPC API Controller...");

        try {
            if (!core_->initialize()) {
                throw ControllerException("Failed to initialize core component");
            }

            running_ = true;
            server_thread_ = std::thread(&GrpcController::runLoop, this);

            return true;
        } catch (const core::CoreException& e) {
            throw ControllerException(std::string("Core error during controller start: ") + e.what());
        }
    }

    bool GrpcController::stop() {
        if (!running_) {
            return true;
        }

        LOG_INFO("Stopping NFOV Camera Controller...");
        running_ = false;

        if (server_thread_.joinable()) {
            server_thread_.join();
        }

        try {
            core_->shutdown();
            LOG_INFO("Camera Controller stopped successfully.");
        } catch (const core::CoreException& e) {
            // FIXME: Is it a good exception handling?
            LOG_ERROR("Error during controller shutdown: {}", e.what());
        }

        return true;
    }

    bool GrpcController::isRunning() const {
        return running_;
    }

    bool GrpcController::setZoom(const double zoom_level) {
        if (!running_) {
            throw ControllerException("Controller not running");
        }

        try {
            core_->setZoom(zoom_level);
            return true;
        } catch (const core::CoreException& e) {
            throw ControllerException(std::string("Core error during zoom operation: ") + e.what());
        }
    }

    double GrpcController::getZoom() const {
        if (!running_) {
            throw ControllerException("Controller not running");
        }

        try {
            return core_->getZoom();
        } catch (const core::CoreException& e) {
            throw ControllerException(std::string("Core error retrieving zoom: ") + e.what());
        }
    }

    bool GrpcController::setFocus(const double focus_value) {
        if (!running_) {
            throw ControllerException("Controller not running");
        }

        try {
            core_->setFocus(focus_value);
            return true;
        } catch (const core::CoreException& e) {
            throw ControllerException(std::string("Core error during focus operation: ") + e.what());
        }
    }

    double GrpcController::getFocus() const {
        if (!running_) {
            throw ControllerException("Controller not running");
        }

        try {
            return core_->getFocus();
        } catch (const core::CoreException& e) {
            throw ControllerException(std::string("Core error retrieving focus: ") + e.what());
        }
    }

    void GrpcController::runLoop() {
        std::string server_address("localhost:50051");
        GrpcImplementation service(this);

        grpc::EnableDefaultHealthCheckService(true);
        grpc::ServerBuilder builder;
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        builder.RegisterService(&service);

        const std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
        LOG_INFO("Service is listening on {}", server_address);

        while (running_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        server->Shutdown();
        LOG_INFO("Service stopped listening");
    }
}
