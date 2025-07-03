#pragma once

class IRegister {
public:
    virtual ~IRegister() = default;
    virtual uint32_t get(uint32_t address) = 0;
    virtual uint8_t set(uint32_t address, uint32_t value) = 0;
};