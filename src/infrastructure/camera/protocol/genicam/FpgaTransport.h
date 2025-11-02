#pragma once

#include "infrastructure/camera/protocol/genicam/include/GenApi/GenApi.h"

namespace camera_service::infrastructure {
    class FpgaTransport final : public GENAPI_NAMESPACE::IPort {
    public:
        void Read(void* buf, int64_t addr, int64_t len) override;
        void Write(const void* buf, int64_t addr, int64_t len) override;
        GENAPI_NAMESPACE::EAccessMode GetAccessMode() const override;
    };
}