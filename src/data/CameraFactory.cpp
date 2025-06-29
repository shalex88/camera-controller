#include "CameraFactory.h"

#include "data/NfovCamera.h"

namespace camera_service::data {
    std::unique_ptr<ICamera> CameraFactory::createCamera(const std::string& camera_type) {
        if (camera_type == "nfov") {
            return std::make_unique<NfovCamera>();
        }

        throw std::invalid_argument("Unknown camera type");
    }
}
