#include "CameraFactory.h"

#include "data/CameraHal.h"
#include "data/NfovCameraHw.h"
#include "data/registers_map_manager/RegisterImplUio.h"
#include "registers_map_manager/RegisterImplFake.h"

namespace camera_service::data {
    std::unique_ptr<ICameraHal> CameraFactory::createCamera(const std::string& camera_type,
                                                            std::shared_ptr<LayerLogger> logger, std::string device) {
        if (camera_type == "nfov") {
            std::unique_ptr<IRegisterImpl> register_impl;
            if (device != "fake") {
                logger->debug("Using device: {}", device);
                register_impl = std::make_unique<RegisterImplUio>(device);
            } else {
                logger->debug("Using fake device");
                register_impl = std::make_unique<RegisterImplFake>();
            }
            auto fpga_manager = std::make_unique<RegistersMapManager>(std::move(register_impl));
            auto camera_hw = std::make_unique<NfovCameraHw>(std::move(fpga_manager));
            return std::make_unique<CameraHal>(std::move(camera_hw), std::move(logger));
        }

        throw std::invalid_argument("Unknown camera type: " + camera_type);
    }
}
