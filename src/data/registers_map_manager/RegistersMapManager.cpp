#include "RegistersMapManager.h"

#include <ranges>

#include "RegistersMap.h"

using namespace camera_service::data;

uint32_t RegistersMapManager::getValue(const REG reg) const {
    std::lock_guard lock(mutex_);
    uint32_t value{};
    if (register_->get(g_registers_map[reg].address, value)) {
        return value;
    }

    return 0xFFFF'FFFF; //FIXME: return Result::Error
}

bool RegistersMapManager::setValue(const REG reg, const uint32_t value) const {
    std::lock_guard lock(mutex_);
    return register_->set(g_registers_map[reg].address, value);
}

bool RegistersMapManager::resetValue(const REG reg) const {
    return setValue(reg, g_registers_map[reg].default_value);
}

bool RegistersMapManager::clearValue(const REG reg) const {
    return setValue(reg, 0);
}

bool RegistersMapManager::setBit(const REG reg, const uint8_t bit_index) const {
    if(bit_index > 31) {
        return false;
    }
    auto reg_value = getValue(reg);
    reg_value |= 1UL << bit_index;

    return setValue(reg, reg_value);
}

bool RegistersMapManager::clearBit(const REG reg, const uint8_t bit_index) const {
    if(bit_index > 31) {
        return false;
    }

    auto reg_value = getValue(reg);
    reg_value &= ~(1UL << bit_index);

    return setValue(reg, reg_value);
}

uint8_t RegistersMapManager::getNibble(const REG reg, const uint8_t nibble_index) const {
    if(nibble_index > 7) {
        return 0xFF; //FIXME: return Result::Error
    }

    const auto reg_value = getValue(reg);

    return (reg_value >> (nibble_index * 4)) & 0xF;
}

uint8_t RegistersMapManager::setNibble(const REG reg, const uint8_t nibble_index, const uint8_t nibble_value) const {
    if(nibble_index > 7 || nibble_value > 0xF) {
        return 0xFF; //FIXME: return Result::Error
    }

    auto reg_value = getValue(reg);
    reg_value = (reg_value & ~(0xF << (nibble_index * 4))) | (nibble_value << (nibble_index * 4));

    return setValue(reg, reg_value);
}

bool RegistersMapManager::resetAll() const {
    for(const auto key: g_registers_map | std::views::keys) {
        if (!setValue(key, g_registers_map[key].default_value)) {
            return false; // Return false if any operation fails
        }
    }
    return true; // Return true if all operations succeed
}

bool RegistersMapManager::clearAll() const {
    for(const auto key: g_registers_map | std::views::keys) {
        if (!setValue(key, 0)) {
            return false; // Return false if any operation fails
        }
    }
    return true; // Return true if all operations succeed
}