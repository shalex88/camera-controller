#pragma once
#include "ICameraHal.h"

#include <memory>

#include "ICameraHw.h"

namespace camera_service::data {
    class CameraHal final : public ICameraHal {
    public:
        explicit CameraHal(std::unique_ptr<ICameraHw> camera_strategy);
        ~CameraHal() override;

        Result<void> setZoom(types::zoom zoom) override;
        Result<types::zoom> getZoom() const override;
        Result<void> setFocus(types::focus focus) override;
        Result<types::focus> getFocus() const override;
        Result<void> connect() override;
        Result<void> disconnect() override;
        bool isConnected() const override;

    private:
        std::unique_ptr<ICameraHw> camera_hw_;
        bool connected_ {false};
        bool isValidZoom(const types::zoom value) const;
        bool isValidFocus(const types::focus value) const;
        types::CameraLimits limits_;
    };
}