#include <gtest/gtest.h>
#include <gmock/gmock.h>
/* Add your project include files here */
#include "data/registers_map_manager/RegistersMapManager.h"
#include <thread>

using namespace camera_service::data;

class RegisterImplMock : public IRegisterImpl {
public:
    MOCK_METHOD(bool, get, (uint32_t, uint32_t&), (const, override));
    MOCK_METHOD(bool, set, (uint32_t, uint32_t), (override));
};

class RegisterMapManagerTest : public testing::Test {
public:
    RegisterMapManagerTest() :
            register_impl(std::make_unique<RegisterImplMock>()),
            register_map(std::make_unique<RegistersMapManager>(std::move(register_impl))) {}
    std::unique_ptr<RegisterImplMock> register_impl;
    //FIXME: register_impl is out of scope, it was moved to register_map
    std::unique_ptr<RegistersMapManager> register_map;
};

TEST_F(RegisterMapManagerTest, GetRegisterValue) {
    EXPECT_CALL(*register_impl.get(), get(testing::_, testing::_))
            .WillOnce(testing::Return(0xFFFF'FFFF));

    EXPECT_EQ(register_map->getValue(REG::SET_ZOOM), 0xFFFF'FFFF);
}

TEST_F(RegisterMapManagerTest, SetRegisterValue) {
    EXPECT_CALL(*register_impl, set(testing::_, 0xFFFF'FFFF))
            .WillOnce(testing::Return(0));

    EXPECT_EQ(register_map->setValue(REG::SET_ZOOM, 0xFFFF'FFFF), 0);
}

TEST_F(RegisterMapManagerTest, ResetRegisterToDefault) {
    EXPECT_CALL(*register_impl, set(testing::_, testing::_))
            .WillOnce(testing::Return(0));

    EXPECT_EQ(register_map->resetValue(REG::SET_ZOOM), 0);
}

TEST_F(RegisterMapManagerTest, ClearRegister) {
    EXPECT_CALL(*register_impl, set(testing::_, testing::_))
            .WillOnce(testing::Return(0));

    EXPECT_EQ(register_map->clearValue(REG::SET_ZOOM), 0);
}

TEST_F(RegisterMapManagerTest, SetBit) {
    EXPECT_CALL(*register_impl, get(testing::_, testing::_))
            .WillOnce(testing::Return(0x0000'0000));
    EXPECT_CALL(*register_impl, set(testing::_, 0x0000'0001))
            .WillOnce(testing::Return(0));

    EXPECT_EQ(register_map->setBit(REG::SET_ZOOM, 0), 0);
}

TEST_F(RegisterMapManagerTest, ClearBit) {
    EXPECT_CALL(*register_impl, get(testing::_, testing::_))
            .WillOnce(testing::Return(0xFFFF'FFFF));
    EXPECT_CALL(*register_impl, set(testing::_, 0xFFFF'FFFE))
            .WillOnce(testing::Return(0));

    EXPECT_EQ(register_map->clearBit(REG::SET_ZOOM, 0), 0);
}

TEST_F(RegisterMapManagerTest, GetOrSetBitLargerThan31) {
    EXPECT_CALL(*register_impl, get(testing::_, testing::_)).Times(0);
    EXPECT_CALL(*register_impl, set(testing::_, testing::_)).Times(0);

    EXPECT_EQ(register_map->setBit(REG::SET_ZOOM, 32), 1);
    EXPECT_EQ(register_map->clearBit(REG::SET_ZOOM, 32), 1);
}

TEST_F(RegisterMapManagerTest, GetNibble) {
    EXPECT_CALL(*register_impl, get(testing::_, testing::_))
            .WillOnce(testing::Return(0xFFFF'FFFF));

    EXPECT_EQ(register_map->getNibble(REG::SET_ZOOM, 0), 0xF);
}

TEST_F(RegisterMapManagerTest, SetNibble) {
    EXPECT_CALL(*register_impl, get(testing::_, testing::_))
            .WillOnce(testing::Return(0x0000'0000));
    EXPECT_CALL(*register_impl, set(testing::_, 0x0000'000F))
            .WillOnce(testing::Return(0));

    EXPECT_EQ(register_map->setNibble(REG::SET_ZOOM, 0, 0xf), 0);
}

TEST_F(RegisterMapManagerTest, GetSetWrongNibbleIndex) {
    EXPECT_CALL(*register_impl, get(testing::_, testing::_)).Times(0);
    EXPECT_CALL(*register_impl, set(testing::_, testing::_)).Times(0);

    EXPECT_EQ(register_map->getNibble(REG::SET_ZOOM, 8), 1);
    EXPECT_EQ(register_map->setNibble(REG::SET_ZOOM, 8, 0xf), 1);
}

TEST_F(RegisterMapManagerTest, SetWrongNibbleValue) {
    EXPECT_CALL(*register_impl, get(testing::_, testing::_)).Times(0);
    EXPECT_CALL(*register_impl, set(testing::_, testing::_)).Times(0);

    EXPECT_EQ(register_map->setNibble(REG::SET_ZOOM, 0, 0xff), 1);
}

TEST_F(RegisterMapManagerTest, ResetAllToDefault) {
    EXPECT_CALL(*register_impl, set(testing::_, testing::_))
            .WillRepeatedly(testing::Return(0));

    EXPECT_EQ(register_map->resetAll(), 0);
}

TEST_F(RegisterMapManagerTest, ClearAllRegisters) {
    EXPECT_CALL(*register_impl, set(testing::_, testing::_))
            .WillRepeatedly(testing::Return(0));

    EXPECT_EQ(register_map->clearAll(), 0);
}

TEST_F(RegisterMapManagerTest, ResetAllToDefaultFails) {
    EXPECT_CALL(*register_impl, set(testing::_, testing::_))
            .WillRepeatedly(testing::Return(0));

    EXPECT_CALL(*register_impl, set(testing::_, testing::_))
            .WillOnce(testing::Return(1));

    EXPECT_EQ(register_map->resetAll(), 1);
}

TEST_F(RegisterMapManagerTest, ClearAllRegistersFails) {
    EXPECT_CALL(*register_impl, set(testing::_, testing::_))
            .WillRepeatedly(testing::Return(0));

    EXPECT_CALL(*register_impl, set(testing::_, testing::_))
            .WillOnce(testing::Return(1));

    EXPECT_EQ(register_map->clearAll(), 1);
}

TEST_F(RegisterMapManagerTest, SetRegisterValueThreadSafety) {
    auto inside_interface_set_func = std::make_shared<std::atomic<bool>>(false);

    ON_CALL(*register_impl, set(testing::_, testing::_))
            .WillByDefault([inside_interface_set_func](int address, uint32_t value) mutable {
                if (*inside_interface_set_func) {
                    return 1;
                }
                *inside_interface_set_func = true;
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                *inside_interface_set_func = false;
                return 0;
            });

    std::atomic error_flag(false);

    std::thread thread1([&] {
        for (int i = 0; i < 1000; ++i) {
            if (register_map->setValue(REG::SET_ZOOM, 0xFFFF'FFFF)) {
                error_flag.store(true);
            }
        }
    });

    std::thread thread2([&] {
        for (int i = 0; i < 1000; ++i) {
            if (register_map->setValue(REG::SET_ZOOM, 0x0000'0000)) {
                error_flag.store(true);
            }
        }
    });

    thread1.join();
    thread2.join();

    EXPECT_EQ(error_flag.load(), false);
}