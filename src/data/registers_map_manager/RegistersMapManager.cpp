#include "RegistersMapManager.h"

#include <ranges>

#include "RegistersMap.h"

uint32_t RegistersMapManager::getValue(const REG reg) {
    std::lock_guard lock(mtx_);
    return register_->get(g_registers_map[reg].address);
}

uint8_t RegistersMapManager::setValue(const REG reg, const uint32_t value) {
    std::lock_guard lock(mtx_);
    return register_->set(g_registers_map[reg].address, value);
}

uint8_t RegistersMapManager::resetValue(const REG reg) {
    return setValue(reg, g_registers_map[reg].default_value);
}

uint8_t RegistersMapManager::clearValue(const REG reg) {
    return setValue(reg, 0);
}

uint8_t RegistersMapManager::setBit(const REG reg, const uint8_t bit_index) {
    if(bit_index > 31) {
        return 1;
    }
    auto reg_value = getValue(reg);
    reg_value |= 1UL << bit_index;

    return setValue(reg, reg_value);
}

uint8_t RegistersMapManager::clearBit(const REG reg, const uint8_t bit_index) {
    if(bit_index > 31) {
        return 1;
    }

    auto reg_value = getValue(reg);
    reg_value &= ~(1UL << bit_index);

    return setValue(reg, reg_value);
}

uint8_t RegistersMapManager::getNibble(const REG reg, const uint8_t nibble_index) {
    if(nibble_index > 7) {
        return 1;
    }

    const auto reg_value = getValue(reg);

    return (reg_value >> (nibble_index * 4)) & 0xF;
}

uint8_t RegistersMapManager::setNibble(const REG reg, const uint8_t nibble_index, const uint8_t nibble_value) {
    if(nibble_index > 7 || nibble_value > 0xF) {
        return 1;
    }

    auto reg_value = getValue(reg);
    reg_value = (reg_value & ~(0xF << (nibble_index * 4))) | (nibble_value << (nibble_index * 4));

    return setValue(reg, reg_value);
}

uint8_t RegistersMapManager::resetAll() {
    uint8_t result{};

    for(const auto key: g_registers_map | std::views::keys) {
        if (setValue(key, g_registers_map[key].default_value)) {
            result++;
            break;
        }
    }

    return result;
}

uint8_t RegistersMapManager::clearAll() {
    uint8_t result{};

    for(const auto key: g_registers_map | std::views::keys) {
        if (setValue(key, 0)) {
            result++;
            break;
        }
    }

    return result;
}