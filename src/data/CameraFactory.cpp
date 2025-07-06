#include "CameraFactory.h"

#include "data/Camera.h"
#include "data/NfovCamera.h"

namespace camera_service::data {
    std::unique_ptr<ICamera> CameraFactory::createCamera(const std::string& camera_type) {
        if (camera_type == "nfov") {
            auto nfov_strategy = std::make_unique<NfovCamera>();
            return std::make_unique<Camera>(std::move(nfov_strategy));
        }

        throw std::invalid_argument("Unknown camera type");
    }
}
