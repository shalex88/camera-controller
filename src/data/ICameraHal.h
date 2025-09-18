#pragma once
#include "common/types/Result.h"
#include "common/types/IZoomCapable.h"
#include "common/types/IFocusCapable.h"
#include "common/types/IInfoCapable.h"

namespace camera_service::data {
    class ICameraHal : public capabilities::IZoomCapable,
                       public capabilities::IFocusCapable,
                       public capabilities::IInfoCapable {
    public:
        ~ICameraHal() override = default;

        virtual Result<void> connect() = 0;
        virtual Result<void> disconnect() = 0;
        virtual bool isConnected() const = 0;
    };
}