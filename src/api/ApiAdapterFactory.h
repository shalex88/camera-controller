#pragma once
#include <memory>
#include <string>
#include "IApiAdapter.h"

namespace camera_service::api {
    class ApiAdapterFactory {
    public:
        static std::unique_ptr<IApiAdapter> createAdapter(const std::string& adapter_type);
    };
}
