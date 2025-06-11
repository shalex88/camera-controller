#include "CameraFactory.h"

#include "NfovCamera.h"

namespace camera_service::data {
    std::unique_ptr<ICamera> CameraFactory::createCamera(const std::string& cameraType) {
        if (cameraType == "nfov") {
            return std::make_unique<NfovCamera>();
        }
        throw CameraException("Unknown camera type");
    }
}
