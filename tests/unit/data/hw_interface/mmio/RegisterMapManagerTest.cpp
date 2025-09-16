#include <gtest/gtest.h>
#include <gmock/gmock.h>
/* Add your project include files here */
#include "data/hw_interface/mmio/RegistersMapManager.h"
#include <thread>

using namespace camera_service::data;

class RegisterImplMock final : public IRegisterImpl {
public:
    MOCK_METHOD(Result<uint32_t>, get, (uint32_t), (const, override));
    MOCK_METHOD(Result<void>, set, (uint32_t, uint32_t), (override));
};

class RegisterMapManagerTest : public testing::Test {
public:
    RegisterMapManagerTest() {
        auto register_impl_obj = std::make_unique<RegisterImplMock>();
        register_impl = register_impl_obj.get();
        register_map = std::make_unique<RegistersMapManager>(std::move(register_impl_obj));
    }
    RegisterImplMock* register_impl{};
    std::unique_ptr<RegistersMapManager> register_map;
};

TEST_F(RegisterMapManagerTest, GetRegisterValue) {
    EXPECT_CALL(*register_impl, get(testing::_))
            .WillOnce(testing::Return(Result<uint32_t>::success(0xFFFF'FFFFu)));

    auto result = register_map->getValue(REG::ZOOM);
    EXPECT_TRUE(result.isSuccess());
    EXPECT_EQ(result.value(), 0xFFFF'FFFFu);
}

TEST_F(RegisterMapManagerTest, SetRegisterValue) {
    EXPECT_CALL(*register_impl, set(testing::_, 0xFFFF'FFFFu))
            .WillOnce(testing::Return(Result<void>::success()));

    auto result = register_map->setValue(REG::ZOOM, 0xFFFF'FFFFu);
    EXPECT_TRUE(result.isSuccess());
}

TEST_F(RegisterMapManagerTest, ResetRegisterToDefault) {
    EXPECT_CALL(*register_impl, set(testing::_, testing::_))
            .WillOnce(testing::Return(Result<void>::success()));

    auto result = register_map->resetValue(REG::ZOOM);
    EXPECT_TRUE(result.isSuccess());
}

TEST_F(RegisterMapManagerTest, ClearRegister) {
    EXPECT_CALL(*register_impl, set(testing::_, testing::_))
            .WillOnce(testing::Return(Result<void>::success()));

    auto result = register_map->clearValue(REG::ZOOM);
    EXPECT_TRUE(result.isSuccess());
}

TEST_F(RegisterMapManagerTest, SetBit) {
    EXPECT_CALL(*register_impl, get(testing::_))
            .WillOnce(testing::Return(Result<uint32_t>::success(0x0000'0000u)));
    EXPECT_CALL(*register_impl, set(testing::_, 0x0000'0001u))
            .WillOnce(testing::Return(Result<void>::success()));

    const auto result = register_map->setBit(REG::ZOOM, 0);
    EXPECT_TRUE(result.isSuccess());
}

TEST_F(RegisterMapManagerTest, ClearBit) {
    EXPECT_CALL(*register_impl, get(testing::_))
            .WillOnce(testing::Return(Result<uint32_t>::success(0xFFFF'FFFFu)));
    EXPECT_CALL(*register_impl, set(testing::_, 0xFFFF'FFFEu))
            .WillOnce(testing::Return(Result<void>::success()));

    auto result = register_map->clearBit(REG::ZOOM, 0);
    EXPECT_TRUE(result.isSuccess());
}

TEST_F(RegisterMapManagerTest, GetOrSetBitLargerThan31) {
    EXPECT_CALL(*register_impl, get(testing::_)).Times(0);
    EXPECT_CALL(*register_impl, set(testing::_, testing::_)).Times(0);

    auto setBitResult = register_map->setBit(REG::ZOOM, 32);
    EXPECT_TRUE(setBitResult.isError());
    EXPECT_EQ(setBitResult.error(), "Bit index out of range (0-31)");

    auto clearBitResult = register_map->clearBit(REG::ZOOM, 32);
    EXPECT_TRUE(clearBitResult.isError());
    EXPECT_EQ(clearBitResult.error(), "Bit index out of range (0-31)");
}

TEST_F(RegisterMapManagerTest, GetNibble) {
    EXPECT_CALL(*register_impl, get(testing::_))
            .WillOnce(testing::Return(Result<uint32_t>::success(0xFFFF'FFFFu)));

    const auto result = register_map->getNibble(REG::ZOOM, 0);
    EXPECT_TRUE(result.isSuccess());
    EXPECT_EQ(result.value(), 0xF);
}

TEST_F(RegisterMapManagerTest, SetNibble) {
    EXPECT_CALL(*register_impl, get(testing::_))
            .WillOnce(testing::Return(Result<uint32_t>::success(0x0000'0000u)));
    EXPECT_CALL(*register_impl, set(testing::_, 0x0000'000Fu))
            .WillOnce(testing::Return(Result<void>::success()));

    const auto result = register_map->setNibble(REG::ZOOM, 0, 0xf);
    EXPECT_TRUE(result.isSuccess());
}

TEST_F(RegisterMapManagerTest, GetSetWrongNibbleIndex) {
    EXPECT_CALL(*register_impl, get(testing::_)).Times(0);
    EXPECT_CALL(*register_impl, set(testing::_, testing::_)).Times(0);

    const auto getNibbleResult = register_map->getNibble(REG::ZOOM, 8);
    EXPECT_TRUE(getNibbleResult.isError());
    EXPECT_EQ(getNibbleResult.error(), "Nibble index out of range (0-7)");

    const auto setNibbleResult = register_map->setNibble(REG::ZOOM, 8, 0xf);
    EXPECT_TRUE(setNibbleResult.isError());
    EXPECT_EQ(setNibbleResult.error(), "Nibble index out of range (0-7)");
}

TEST_F(RegisterMapManagerTest, SetWrongNibbleValue) {
    EXPECT_CALL(*register_impl, get(testing::_)).Times(0);
    EXPECT_CALL(*register_impl, set(testing::_, testing::_)).Times(0);

    const auto result = register_map->setNibble(REG::ZOOM, 0, 0xff);
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error(), "Nibble value out of range (0-15)");
}

TEST_F(RegisterMapManagerTest, ResetAllToDefault) {
    EXPECT_CALL(*register_impl, set(testing::_, testing::_))
            .WillRepeatedly(testing::Return(Result<void>::success()));

    const auto result = register_map->resetAll();
    EXPECT_TRUE(result.isSuccess());
}

TEST_F(RegisterMapManagerTest, ClearAllRegisters) {
    EXPECT_CALL(*register_impl, set(testing::_, testing::_))
            .WillRepeatedly(testing::Return(Result<void>::success()));

    const auto result = register_map->clearAll();
    EXPECT_TRUE(result.isSuccess());
}

TEST_F(RegisterMapManagerTest, ResetAllToDefaultFails) {
    EXPECT_CALL(*register_impl, set(testing::_, testing::_))
            .WillRepeatedly(testing::Return(Result<void>::error("Register access failed")));

    const auto result = register_map->resetAll();
    EXPECT_TRUE(result.isError());
    EXPECT_THAT(result.error(), testing::HasSubstr("Failed to reset register"));
}

TEST_F(RegisterMapManagerTest, ClearAllRegistersFails) {
    EXPECT_CALL(*register_impl, set(testing::_, testing::_))
            .WillRepeatedly(testing::Return(Result<void>::error("Register access failed")));

    const auto result = register_map->clearAll();
    EXPECT_TRUE(result.isError());
    EXPECT_THAT(result.error(), testing::HasSubstr("Failed to clear register"));
}

TEST_F(RegisterMapManagerTest, SetRegisterValueThreadSafety) {
    auto inside_interface_set_func = std::make_shared<std::atomic<bool>>(false);

    ON_CALL(*register_impl, set(testing::_, testing::_))
            .WillByDefault([inside_interface_set_func](uint32_t address, uint32_t value) mutable {
                if (*inside_interface_set_func) {
                    return Result<void>::error("Concurrent access detected");
                }
                *inside_interface_set_func = true;
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                *inside_interface_set_func = false;
                return Result<void>::success();
            });

    std::atomic error_flag(false);

    std::thread thread1([&] {
        for (int i = 0; i < 1000; ++i) {
            const auto result = register_map->setValue(REG::ZOOM, 0xFFFF'FFFF);
            if (result.isError()) {
                error_flag.store(true);
            }
        }
    });

    std::thread thread2([&] {
        for (int i = 0; i < 1000; ++i) {
            const auto result = register_map->setValue(REG::ZOOM, 0x0000'0000);
            if (result.isError()) {
                error_flag.store(true);
            }
        }
    });

    thread1.join();
    thread2.join();

    EXPECT_EQ(error_flag.load(), false);
}