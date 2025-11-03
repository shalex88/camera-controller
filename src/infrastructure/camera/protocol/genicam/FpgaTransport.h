#pragma once

#include "infrastructure/camera/protocol/genicam/include/GenApi/GenApi.h"

namespace camera_service::infrastructure {
    class FpgaTransport final : public GENAPI_NAMESPACE::IPort {
    public:
        explicit FpgaTransport(const std::string& device);
        ~FpgaTransport() override;

        // Custom GenCP transport for FPGA communication
        void Read(void* buffer, int64_t address, int64_t length) override;
        void Write(const void* buffer, int64_t address, int64_t length) override;
        GENAPI_NAMESPACE::EAccessMode GetAccessMode() const override;

    private:
        enum class Target : uint32_t {
            Host = 0x80000000,
            Camera = 0x00000000
        };
        int fd_{-1};
        uint32_t* regs_{};
        std::string device_{};

        bool open();
        void close();
        bool configureLink() const;
        void writeTransaction(uint32_t target_reg, uint32_t value) const;
        uint32_t readTransaction(uint32_t target_reg) const;
        void writeReg(Target target, auto reg, uint32_t value) const;
        uint32_t readReg(Target target, auto reg) const;
    };
}