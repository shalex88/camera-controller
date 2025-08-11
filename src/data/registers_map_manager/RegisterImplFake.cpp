#include "RegisterImplFake.h"

uint32_t RegisterImplFake::get(uint32_t address) {
    return reg_value_;
};

uint8_t RegisterImplFake::set(uint32_t address, uint32_t value) {
    reg_value_ = value;
    return 0;
}