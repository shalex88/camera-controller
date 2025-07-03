#pragma once

#include <memory>
#include <utility>
#include <mutex>
#include "data/registers_map_manager/IRegister.h"
#include "RegistersMap.h"

class RegistersMapManager {
public:
    explicit RegistersMapManager(std::shared_ptr<IRegister> reg_interface)
        : register_interface_(std::move(reg_interface)) {
    }
    ~RegistersMapManager() = default;

    [[nodiscard]] uint32_t getValue(REG reg);
    [[nodiscard]] uint8_t setValue(REG reg, uint32_t value);
    [[nodiscard]] uint8_t resetValue(REG reg);
    [[nodiscard]] uint8_t clearValue(REG reg);
    [[nodiscard]] uint8_t setBit(REG reg, uint8_t bit_index);
    [[nodiscard]] uint8_t clearBit(REG reg, uint8_t bit_index);
    [[nodiscard]] uint8_t getNibble(REG reg, uint8_t nibble_index);
    [[nodiscard]] uint8_t setNibble(REG reg, uint8_t nibble_index, uint8_t nibble_value);
    [[nodiscard]] uint8_t resetAll();
    [[nodiscard]] uint8_t clearAll();
private:
    std::shared_ptr<IRegister> register_interface_;
    std::mutex mtx_;
};