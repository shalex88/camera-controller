#include "CameraFactory.h"

#include "common/config/ConfigManager.h"
#include "common/logger/Logger.h"
#include "infrastructure/camera/devices/AdimecCamera.h"
#include "infrastructure/camera/devices/FakeAdvancedCamera.h"
#include "infrastructure/camera/devices/FakeSimpleCamera.h"
#include "infrastructure/camera/devices/MwirCamera.h"
#include "infrastructure/camera/devices/SonyCamera.h"
#include "infrastructure/camera/hal/Camera.h"
#include "infrastructure/camera/hal/ICamera.h"
#include "infrastructure/camera/protocol/itl/ItlProtocol.h"
#include "infrastructure/camera/protocol/visca/ViscaProtocol.h"
#include "infrastructure/camera/transport/ethernet/TcpClient.h"
#include "infrastructure/camera/transport/mmio/RegisterImplUio.h"
#include "infrastructure/camera/transport/uart/Uart.h"
#include "infrastructure/camera/transport/mmio/RegistersMapManager.h"

namespace camera_service::infrastructure {
    std::unique_ptr<ICamera> CameraFactory::createCamera(const common::DataConfig& config) {
        LOG_DEBUG("Device: {} {}", config.camera, config.device);

        if (config.camera == "adimec") {
            auto transport = std::make_unique<RegisterImplUio>(config.device);
            auto protocol = std::make_unique<RegistersMapManager>(std::move(transport));
            auto camera = std::make_unique<AdimecCamera>(std::move(protocol));
            return std::make_unique<Camera>(std::move(camera));
        }

        if (config.camera == "sony") {
            auto transport = std::make_unique<Uart>(config.device);
            auto protocol = std::make_unique<ViscaProtocol>(std::move(transport));
            auto camera = std::make_unique<SonyCamera>(std::move(protocol));
            return std::make_unique<Camera>(std::move(camera));
        }

        if (config.camera == "mwir") {
            auto transport = std::make_unique<TcpClient>(config.device);
            auto protocol = std::make_unique<ItlProtocol>(std::move(transport));
            auto camera = std::make_unique<MwirCamera>(std::move(protocol));
            return std::make_unique<Camera>(std::move(camera));
        }

        if (config.camera == "fake_advanced") {
            auto camera = std::make_unique<FakeAdvancedCamera>();
            return std::make_unique<Camera>(std::move(camera));
        }

        if (config.camera == "fake_simple") {
            auto camera = std::make_unique<FakeSimpleCamera>();
            return std::make_unique<Camera>(std::move(camera));
        }

        throw std::invalid_argument("Unknown camera type: " + config.camera);
    }
}
