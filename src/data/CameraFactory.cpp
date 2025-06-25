#include "CameraFactory.h"

#include "NfovCamera.h"
#include "WfovCamera.h"

namespace camera_service::data {
    std::unique_ptr<ICamera> CameraFactory::createCamera(const std::string& camera_type) {
        if (camera_type == "nfov") {
            return std::make_unique<NfovCamera>();
        }

        if (camera_type == "wfov") {
            return std::make_unique<WfovCamera>();
        }
        throw std::invalid_argument("Unknown camera type");
    }
}
