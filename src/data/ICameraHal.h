#pragma once
#include "common/types/Result.h"
#include "common/types/ICameraCapabilities.h"

namespace camera_service::data {
    class ICameraHal : public api::ICameraCapabilities {
    public:
        ~ICameraHal() override = default;

        virtual Result<void> connect() = 0;
        virtual Result<void> disconnect() = 0;
        virtual bool isConnected() const = 0;
    };
}