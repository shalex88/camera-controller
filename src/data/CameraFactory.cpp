#include "CameraFactory.h"

#include "data/CameraHal.h"
#include "data/camera/AdimecCamera.h"
#include "data/camera/SonyCamera.h"
#include "data/camera/MwirCamera.h"
#include "data/camera/FakeAdvancedCamera.h"
#include "data/camera/FakeSimpleCamera.h"
#include "data/transport/mmio/RegisterImplUio.h"
#include "data/transport/ethernet/TcpClient.h"
#include "data/transport/ethernet/ItlProtocol.h"

namespace camera_service::data {
    std::unique_ptr<ICameraHal> CameraFactory::createCamera(
        std::shared_ptr<LayerLogger> logger, const DataConfig& config) {
        logger->info("Camera: {}", config.camera);
        logger->debug("Device: {}", config.device);

        if (config.camera == "adimec") {
            auto transport = std::make_unique<RegisterImplUio>(config.device);
            auto protocol = std::make_unique<RegistersMapManager>(std::move(transport));
            auto camera = std::make_unique<AdimecCamera>(std::move(protocol));
            return std::make_unique<CameraHal>(std::move(camera), std::move(logger));
        }

        if (config.camera == "sony") {
            auto camera = std::make_unique<SonyCamera>(config.device);
            return std::make_unique<CameraHal>(std::move(camera), std::move(logger));
        }

        if (config.camera == "mwir") {
            auto transport = std::make_unique<TcpClient>(config.device);
            auto protocol = std::make_unique<ItlProtocol>(std::move(transport));
            auto camera = std::make_unique<MwirCamera>(std::move(protocol));
            return std::make_unique<CameraHal>(std::move(camera), std::move(logger));
        }

        if (config.camera == "fake_advanced") {
            auto camera = std::make_unique<FakeAdvancedCamera>();
            return std::make_unique<CameraHal>(std::move(camera), std::move(logger));
        }

        if (config.camera == "fake_simple") {
            auto camera = std::make_unique<FakeSimpleCamera>();
            return std::make_unique<CameraHal>(std::move(camera), std::move(logger));
        }

        throw std::invalid_argument("Unknown camera type: " + config.camera);
    }
}