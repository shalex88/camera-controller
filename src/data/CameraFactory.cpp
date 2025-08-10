#include "CameraFactory.h"

#include "data/CameraHal.h"
#include "data/NfovCameraHw.h"

namespace camera_service::data {
    std::unique_ptr<ICameraHal> CameraFactory::createCamera(const std::string& camera_type,
                                                           std::shared_ptr<LayerLogger> logger) {
        if (camera_type == "nfov") {
            auto camera_hw = std::make_unique<NfovCameraHw>();
            return std::make_unique<CameraHal>(std::move(camera_hw), std::move(logger));
        }

        throw std::invalid_argument("Unknown camera type: " + camera_type);
    }
}
