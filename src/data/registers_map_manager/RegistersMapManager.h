#pragma once

#include <memory>
#include <utility>
#include <mutex>

#include "data/registers_map_manager/IRegisterImpl.h"

#include "RegistersMap.h"

class RegistersMapManager {
public:
    explicit RegistersMapManager(std::unique_ptr<IRegisterImpl> register_impl)
        : register_(std::move(register_impl)) {
    }
    ~RegistersMapManager() = default;

    uint32_t getValue(REG reg);
    uint8_t setValue(REG reg, uint32_t value);
    uint8_t resetValue(REG reg);
    uint8_t clearValue(REG reg);
    uint8_t setBit(REG reg, uint8_t bit_index);
    uint8_t clearBit(REG reg, uint8_t bit_index);
    uint8_t getNibble(REG reg, uint8_t nibble_index);
    uint8_t setNibble(REG reg, uint8_t nibble_index, uint8_t nibble_value);
    uint8_t resetAll();
    uint8_t clearAll();

private:
    std::unique_ptr<IRegisterImpl> register_;
    std::mutex mtx_;
};