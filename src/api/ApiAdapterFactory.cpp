#include "ApiAdapterFactory.h"
#include "GrpcAdapter.h"

namespace camera_service::api {
    std::unique_ptr<IApiAdapter> ApiAdapterFactory::createAdapter(const std::string& adapter_type) {
        if (adapter_type == "grpc") {
            return std::make_unique<GrpcAdapter>();
        }
        throw std::runtime_error("Unknown adapter type: " + adapter_type);
    }
}
