#pragma once

#include "IRegisterImpl.h"

class RegisterImplFake final : public camera_service::data::IRegisterImpl {
public:
    ~RegisterImplFake() override = default;
    Result<void> set(uint32_t address, uint32_t value) override;
    Result<uint32_t> get(uint32_t address) const override;

private:
    uint32_t reg_value_{};
};