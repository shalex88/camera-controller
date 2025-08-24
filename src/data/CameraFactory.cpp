#include "CameraFactory.h"

#include "data/CameraHal.h"
#include "data/NfovCameraHw.h"
#include "data/registers_map_manager/RegisterImplUio.h"

namespace camera_service::data {
    std::unique_ptr<ICameraHal> CameraFactory::createCamera(const std::string& camera_type,
                                                           std::shared_ptr<LayerLogger> logger) {
        if (camera_type == "nfov") {
            auto register_impl = std::make_unique<RegisterImplUio>("/dev/uio0");
            auto fpga_manager = std::make_unique<RegistersMapManager>(std::move(register_impl));
            auto camera_hw = std::make_unique<NfovCameraHw>(std::move(fpga_manager));
            return std::make_unique<CameraHal>(std::move(camera_hw), std::move(logger));
        }

        throw std::invalid_argument("Unknown camera type: " + camera_type);
    }
}
