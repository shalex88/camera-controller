#include "CameraFactory.h"

#include "data/CameraHal.h"
#include "data/camera/AdimecCamera.h"
#include "data/camera/SonyCamera.h"
#include "data/camera/SonyCameraVisca.h"
#include "data/hw_interface/mmio/RegisterImplUio.h"
#include "data/hw_interface/mmio/RegisterImplFake.h"
#include "data/hw_interface/uart/UartInterface.h"
#include "data/hw_interface/uart/FakeUartInterface.h"

namespace camera_service::data {
    std::unique_ptr<ICameraHal> CameraFactory::createCamera(
        std::shared_ptr<LayerLogger> logger, const DataConfig& config) {
        logger->info("Camera: {}", config.camera);
        logger->debug("Device: {}", config.device);

        if (config.camera == "adimec") {
            std::unique_ptr<IRegisterImpl> register_impl;

            if (config.device != "fake") {
                register_impl = std::make_unique<RegisterImplUio>(config.device);
            } else {
                register_impl = std::make_unique<RegisterImplFake>();
            }

            auto fpga_manager = std::make_unique<RegistersMapManager>(std::move(register_impl));
            auto camera_hw = std::make_unique<AdimecCamera>(std::move(fpga_manager));
            return std::make_unique<CameraHal>(std::move(camera_hw), std::move(logger));
        }

        if (config.camera == "sony") {
            std::unique_ptr<uart::IUartInterface> uart_interface;

            if (config.device != "fake") {
                uart_interface = std::make_unique<uart::UartInterface>(config.device);
            } else {
                uart_interface = std::make_unique<uart::FakeUartInterface>();
            }

            auto camera_hw = std::make_unique<SonyCamera>(std::move(uart_interface));
            return std::make_unique<CameraHal>(std::move(camera_hw), std::move(logger));
        }

        if (config.camera == "sony-visca") {
            auto camera_hw = std::make_unique<SonyCameraVisca>(config.device);
            return std::make_unique<CameraHal>(std::move(camera_hw), std::move(logger));
        }

        throw std::invalid_argument("Unknown camera type: " + config.camera);
    }
}