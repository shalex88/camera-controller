#include "CameraFactory.h"

#include "data/CameraHal.h"
#include "data/NfovCameraHw.h"

namespace camera_service::data {
    std::unique_ptr<ICameraHal> CameraFactory::createCamera(const std::string& camera_type) {
        if (camera_type == "nfov") {
            auto nfov_strategy = std::make_unique<NfovCameraHw>();
            return std::make_unique<CameraHal>(std::move(nfov_strategy));
        }

        throw std::invalid_argument("Unknown camera type");
    }
}
