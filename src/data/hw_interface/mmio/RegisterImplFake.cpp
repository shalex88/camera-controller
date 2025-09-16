#include "RegisterImplFake.h"

Result<uint32_t> RegisterImplFake::get(uint32_t address) const {
    return Result<uint32_t>::success(reg_value_);
}

Result<void> RegisterImplFake::set(uint32_t address, uint32_t value) {
    reg_value_ = value;
    return Result<void>::success();
}