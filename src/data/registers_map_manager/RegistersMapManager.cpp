#include "RegistersMapManager.h"

#include "RegistersMap.h"

uint32_t RegistersMapManager::getValue(REG reg) {
    std::lock_guard<std::mutex> lock(mtx_);
    return register_interface_->get(g_registers_map[reg].address);
}

uint8_t RegistersMapManager::setValue(REG reg, uint32_t value) {
    std::lock_guard<std::mutex> lock(mtx_);
    return register_interface_->set(g_registers_map[reg].address, value);
}

uint8_t RegistersMapManager::resetValue(REG reg) {
    return setValue(reg, g_registers_map[reg].default_value);
}

uint8_t RegistersMapManager::clearValue(REG reg) {
    return setValue(reg, 0);
}

uint8_t RegistersMapManager::setBit(REG reg, uint8_t bit_index) {
    if(bit_index > 31) {
        return 1;
    }
    auto reg_value = getValue(reg);
    reg_value |= 1UL << bit_index;

    return setValue(reg, reg_value);
}

uint8_t RegistersMapManager::clearBit(REG reg, uint8_t bit_index) {
    if(bit_index > 31) {
        return 1;
    }

    auto reg_value = getValue(reg);
    reg_value &= ~(1UL << bit_index);

    return setValue(reg, reg_value);
}

uint8_t RegistersMapManager::getNibble(REG reg, uint8_t nibble_index) {
    if(nibble_index > 7) {
        return 1;
    }

    auto reg_value = getValue(reg);

    return (reg_value >> (nibble_index * 4)) & 0xF;
}

uint8_t RegistersMapManager::setNibble(REG reg, uint8_t nibble_index, uint8_t nibble_value) {
    if(nibble_index > 7 || nibble_value > 0xF) {
        return 1;
    }

    auto reg_value = getValue(reg);
    reg_value = (reg_value & ~(0xF << (nibble_index * 4))) | (nibble_value << (nibble_index * 4));

    return setValue(reg, reg_value);
}

uint8_t RegistersMapManager::resetAll() {
    uint8_t result{};

    for(auto reg : g_registers_map) {
        uint8_t error = setValue(reg.first, g_registers_map[reg.first].default_value);
        if (error) {
            result++;
            break;
        }
    }

    return result;
}

uint8_t RegistersMapManager::clearAll() {
    uint8_t result{};

    for(auto reg : g_registers_map) {
        uint8_t error = setValue(reg.first, 0);
        if (error) {
            result++;
            break;
        }
    }

    return result;
}