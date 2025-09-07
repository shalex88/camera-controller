#pragma once

#include "IRegisterImpl.h"

class RegisterImplFake final : public camera_service::data::IRegisterImpl {
public:
    ~RegisterImplFake() override = default;
    bool set(uint32_t address, uint32_t value) override;
    bool get(uint32_t address, uint32_t& value) const override;

private:
    uint32_t reg_value_{};
};