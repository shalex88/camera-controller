#pragma once

#include "IRegisterImpl.h"

class RegisterImplFake final : public IRegisterImpl {
public:
    ~RegisterImplFake() override = default;
    uint32_t get(uint32_t address) override;
    uint8_t set(uint32_t address, uint32_t value) override;
private:
    uint32_t reg_value_{};
};