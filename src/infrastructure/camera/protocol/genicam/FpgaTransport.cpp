#include "FpgaTransport.h"

#include <fcntl.h>
#include <sys/mman.h>

#include "common/logger/Logger.h"

namespace {
    constexpr uint32_t FPGA_BASE_ADDR = 0x90000000;
    constexpr uint32_t FPGA_MEMORY_SIZE = 0x4000000;
    enum class FpgaRegs : uint32_t {
        Trigger = 0x00,
        Channel = 0x04,
        TargetReg = 0x0C,
        State = 0x10,
        AddressType = 0x14,
        ReadbackData = 0x08
    };

    enum class HostReg : uint32_t {
        SelectChannel = 0x0,
        WorkingSpeed = 0x4,
        LinkStatus = 0x8,
        Reset = 0x2000,
        StreamID = 0x2018,
        CameraIndex = 0x40,
        CameraArbitration = 0x3C,
        HostDecoder = 0x2034
    };
}

namespace camera_service::infrastructure {
    FpgaTransport::FpgaTransport(const std::string& device)
        : device_(device) {
        if (!open()) {
            LOG_ERROR("Failed to open FPGA transport on device: {}", device);
        }
    }

    FpgaTransport::~FpgaTransport() {
        close();
    }

    void FpgaTransport::Read(void* buffer, const int64_t address, const int64_t length) {
        const uint32_t val = readReg(Target::Camera, static_cast<uint32_t>(address));
        std::memcpy(buffer, &val, std::min<int64_t>(length, 4));
    }

    void FpgaTransport::Write(const void* buffer, const int64_t address, const int64_t length) {
        uint32_t val = 0;
        std::memcpy(&val, buffer, std::min<int64_t>(length, 4));
        writeReg(Target::Camera, static_cast<uint32_t>(address), val);
    }

    GENAPI_NAMESPACE::EAccessMode FpgaTransport::GetAccessMode() const {
        return GENAPI_NAMESPACE::RW;
    }

    bool FpgaTransport::open() {
        fd_ = ::open(device_.c_str(), O_RDWR | O_SYNC);
        if (fd_ < 0) {
            return false;
        }

        regs_ = static_cast<uint32_t*>(mmap(nullptr, FPGA_MEMORY_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd_,
                                                     FPGA_BASE_ADDR));
        if (regs_ == MAP_FAILED) {
            regs_ = nullptr;
            return false;
        }

        if (!configureLink()) {
            close();
            return false;
        }

        return true;
    }

    void FpgaTransport::close() {
        if (regs_) {
            munmap(regs_, FPGA_MEMORY_SIZE);
        }
        if (fd_ >= 0) {
            ::close(fd_);
        }
        regs_ = nullptr;
    }

    bool FpgaTransport::configureLink() const {
        uint32_t result{};

        // [CXP] Channel select 0
        writeReg(Target::Host, HostReg::SelectChannel, 0x0);

        // [CXP] Link speed discovery 3.125 Gbps
        writeReg(Target::Host, HostReg::WorkingSpeed, 0x38);

        // [CXP] Reset link on camera side
        writeReg(Target::Camera, 0x4000, 0x1);

        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        // [CXP] Read link status (bit0=1 expected)
        result = readReg(Target::Host, HostReg::LinkStatus);
        LOG_INFO("Link status: 0x{:X}", result);

        // [CXP] Read magic number (expect 0xC0A79AE5)
        result = readReg(Target::Camera, 0x0);
        LOG_INFO("Camera magic: 0x{:X}", result);

        // [CXP] Set link speed to camera 3.125 Gbps
        writeReg(Target::Camera, 0x4014, 0x38);

        // [CXP] Confirm link speed discovery again
        writeReg(Target::Host, HostReg::WorkingSpeed, 0x38);

        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        // [CXP] Read link status (bits0:1 = 11 expected)
        result = readReg(Target::Host, HostReg::LinkStatus);
        LOG_INFO("Link status: 0x{:X}", result);

        // [CXP] Read magic again
        result = readReg(Target::Camera, 0x0);
        LOG_INFO("Camera magic: 0x{:X}", result);

        // [CXP] Read stream ID (0x301C)
        result = readReg(Target::Camera, 0x301C);
        LOG_INFO("Stream ID: 0x{:X}", result);

        // [CXP] Read AcquisitionStartAddress (0x300C)
        result = readReg(Target::Camera, 0x300C);
        LOG_INFO("AcquisitionStartAddress: 0x{:X}", result);

        // [CXP] Write MasterHostConnectionID = 0xDE000000
        writeReg(Target::Camera, 0x4008, 0xDE000000);

        // [CXP] Packet size = 2048
        writeReg(Target::Camera, 0x4010, 0x800);

        // [CXP] Decoder reset
        writeReg(Target::Host, HostReg::Reset, 0x2);

        // [CXP] Stream id[15..8]=1, channel mask[7..0]=1
        writeReg(Target::Host, HostReg::StreamID, 0x00000100);

        // [CXP] Select camera 0
        writeReg(Target::Host, HostReg::CameraIndex, 0x0);

        // [CXP] Connect link0->arbiter0
        writeReg(Target::Host, HostReg::CameraArbitration, 0x1);

        // [CXP] Connect arbiter0->decoder0
        writeReg(Target::Host, HostReg::HostDecoder, 0x0);

        // [CXP] Decoder enable
        writeReg(Target::Host, HostReg::Reset, 0x1);

        // [CXP] Start acquisition
        writeReg(Target::Camera, 0x1005C, 0x1);

        return true; //TODO: add failure checks
    }

    void FpgaTransport::writeTransaction(const uint32_t target_reg, const uint32_t value) const {
        regs_[static_cast<uint32_t>(FpgaRegs::Trigger) / 4] = 0;
        regs_[static_cast<uint32_t>(FpgaRegs::Channel) / 4] = 1;
        regs_[static_cast<uint32_t>(FpgaRegs::AddressType) / 4] = 0x2;
        regs_[static_cast<uint32_t>(FpgaRegs::TargetReg) / 4] = target_reg;
        regs_[static_cast<uint32_t>(FpgaRegs::State) / 4] = 0;
        regs_[static_cast<uint32_t>(FpgaRegs::TargetReg) / 4] = value;
        regs_[static_cast<uint32_t>(FpgaRegs::State) / 4] = 1;
        regs_[static_cast<uint32_t>(FpgaRegs::Trigger) / 4] = 1;
    }

    uint32_t FpgaTransport::readTransaction(const uint32_t target_reg) const {
        regs_[static_cast<uint32_t>(FpgaRegs::Trigger) / 4] = 0;
        regs_[static_cast<uint32_t>(FpgaRegs::Channel) / 4] = 0;
        regs_[static_cast<uint32_t>(FpgaRegs::TargetReg) / 4] = target_reg;
        regs_[static_cast<uint32_t>(FpgaRegs::State) / 4] = 0;
        regs_[static_cast<uint32_t>(FpgaRegs::Trigger) / 4] = 1;


        return regs_[static_cast<uint32_t>(FpgaRegs::ReadbackData) / 4];
    }

    void FpgaTransport::writeReg(const Target target, const auto reg, const uint32_t value) const {
        const auto targer_reg = static_cast<uint32_t>(target) + static_cast<uint32_t>(reg);
        writeTransaction(targer_reg, value);
        LOG_INFO("{} = {}", targer_reg, value);
    }

    uint32_t FpgaTransport::readReg(const Target target, const auto reg) const {
        const auto target_reg = static_cast<uint32_t>(target) + static_cast<uint32_t>(reg);
        const uint32_t value = readTransaction(target_reg);
        LOG_INFO("{} = {}", target_reg, value);

        return value;
    }
}