#include "RegisterImplFake.h"

bool RegisterImplFake::get(uint32_t address, uint32_t& value) const {
    value = reg_value_;
    return true;
};

bool RegisterImplFake::set(uint32_t address, uint32_t value) {
    reg_value_ = value;
    return true;
}