#include "Visca.h"

#include <algorithm>
#include <unistd.h>
#include <sys/ioctl.h>

namespace camera_service::infrastructure {
    ErrorCode Visca::write(const ViscaPacket* packet) const {
        if (const auto result = transport_->write(std::span<const std::byte>(reinterpret_cast<const std::byte*>(packet->bytes), packet->size)); result.isError()) {
            return ErrorCode::Failure;
        }
        return ErrorCode::Success;
    }

    ErrorCode Visca::sendPacket(ViscaPacket* packet) const {
        // check data:
        if ((transport_->iface.address > 7) || (camera.address > 7) || (transport_->iface.broadcast > 1)) {
            return ErrorCode::Failure;
        }

        // build header:
        packet->bytes[0] = 0x80;
        packet->bytes[0] |= (transport_->iface.address << 4);
        if (transport_->iface.broadcast > 0) {
            packet->bytes[0] |= (transport_->iface.broadcast << 3);
            packet->bytes[0] &= 0xF8;
        } else {
            packet->bytes[0] |= camera.address;
        }

        // append footer
        appendByte(packet, VISCA_TERMINATOR);

        return write(packet);
    }

    ErrorCode Visca::read() const {
        auto result = transport_->read();
        if (result.isError()) {
            return ErrorCode::Failure;
        }

        auto data = std::move(result).value();
        if (data.empty()) {
            return ErrorCode::Failure;
        }

        // Find the terminator in the received data
        const auto terminator_it = std::ranges::find(data, static_cast<std::byte>(VISCA_TERMINATOR));
        if (terminator_it == data.end()) {
            // No terminator found, this shouldn't happen in a valid VISCA message
            return ErrorCode::Failure;
        }

        // Calculate the number of bytes including the terminator
        transport_->iface.size = std::distance(data.begin(), terminator_it) + 1;

        // Copy data to ibuf
        for (size_t i = 0; i < transport_->iface.size && i < sizeof(transport_->iface.ibuf); ++i) {
            transport_->iface.ibuf[i] = static_cast<uint8_t>(data[i]);
        }

        return ErrorCode::Success;
    }

    /***********************************/
    /*       SYSTEM  FUNCTIONS         */
    /***********************************/

    Result<void> Visca::open() const {
        return transport_->open();
    }

    Result<void> Visca::close() const {
        return transport_->close();
    }

    void Visca::appendByte(ViscaPacket* packet, const uint8_t byte) {
        packet->bytes[packet->size] = byte;
        (packet->size)++;
    }

    void Visca::initPacket(ViscaPacket* packet) {
        // we start writing at byte 1, the first byte will be filled by the
        // packet sending function. This function will also append a terminator.
        packet->size = 1;
    }

    ErrorCode Visca::getReply() const {
        // first message: -------------------
        if (read() != ErrorCode::Success) {
            return ErrorCode::Failure;
        }
        transport_->iface.type = static_cast<ResponseType>(transport_->iface.ibuf[1] & 0xF0);

        // skip ack messages
        while (transport_->iface.type == ResponseType::Ack) {
            if (read() != ErrorCode::Success) {
                return ErrorCode::Failure;
            }
            transport_->iface.type = static_cast<ResponseType>(transport_->iface.ibuf[1] & 0xF0);
        }

        switch (transport_->iface.type) {
            case ResponseType::Clear:
            case ResponseType::Address:
            case ResponseType::Completed:
            case ResponseType::Error:
                return ErrorCode::Success;
                break;
            default:
                return ErrorCode::Failure;
        }
    }

    ErrorCode Visca::sendPacketWithReply(ViscaPacket* packet) const {
        if (sendPacket(packet) != ErrorCode::Success) {
            return ErrorCode::Failure;
        }

        if (getReply() != ErrorCode::Success) {
            return ErrorCode::Failure;
        }

        return ErrorCode::Success;
    }

    ErrorCode Visca::unreadBytes(const unsigned char* buffer, uint32_t* buffer_size) const {
        uint32_t bytes = 0;
        *buffer_size = 0;

        ioctl(transport_->iface.port_fd, FIONREAD, &bytes);
        if (bytes > 0) {
            bytes = (bytes > *buffer_size) ? *buffer_size : bytes;
            ::read(transport_->iface.port_fd, &buffer, bytes);
            *buffer_size = bytes;
            return ErrorCode::Failure;
        }
        return ErrorCode::Success;
    }

    /****************************************************************************/
    /*                           PUBLIC FUNCTIONS                               */
    /****************************************************************************/

    /***********************************/
    /*       SYSTEM  FUNCTIONS         */
    /***********************************/

    Visca::Visca(std::unique_ptr<Uart> transport)
        : transport_(std::move(transport)) {}

    Result<void> Visca::setAddress() {
        ViscaPacket packet{};
        int32_t camera_num = 0;

        const auto backup = transport_->iface.broadcast;

        initPacket(&packet);
        appendByte(&packet, 0x30);
        appendByte(&packet, 0x01);

        transport_->iface.broadcast = 1;
        if (sendPacket(&packet) != ErrorCode::Success) {
            transport_->iface.broadcast = backup;
            return Result<void>::error("Failed to send setAddress command");
        }
        transport_->iface.broadcast = backup;

        if (getReply() != ErrorCode::Success) {
            return Result<void>::error("Failed to get reply for setAddress command");
        }
        /* We parse the message from the camera here  */
        /* We expect to receive 4*camera_num bytes,
               every packet should be 88 30 0x FF, x being
               the camera id+1. The number of cams will thus be
               ibuf[bytes-2]-1  */
        if ((transport_->iface.size & 0x3) != 0) {
            /* check multiple of 4 */
            return Result<void>::error("Invalid response length for setAddress command");
        }
        camera_num = transport_->iface.ibuf[transport_->iface.size - 2] - 1;
        if ((camera_num == 0) || (camera_num > 7)) {
            return Result<void>::error("Invalid number of cameras detected");
        }
        camera.address = camera_num;
        return Result<void>::success();
    }

    Result<void> Visca::clear() const {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, 0x01);
        appendByte(&packet, 0x00);
        appendByte(&packet, 0x01);

        if (sendPacket(&packet) != ErrorCode::Success) {
            return Result<void>::error("Failed to send clear command");
        }
        if (getReply() != ErrorCode::Success) {
            return Result<void>::error("Failed to get reply for clear command");
        }
        return Result<void>::success();
    }

    Result<std::string> Visca::getCameraInfo() {
        ViscaPacket packet{};
        packet.bytes[0] = 0x80 | camera.address;
        packet.bytes[1] = 0x09;
        packet.bytes[2] = 0x00;
        packet.bytes[3] = 0x02;
        packet.bytes[4] = VISCA_TERMINATOR;
        packet.size = 5;

        if (write(&packet) != ErrorCode::Success) {
            return Result<std::string>::error("Failed to write getCameraInfo command");
        }
        if (getReply() != ErrorCode::Success) {
            return Result<std::string>::error("Failed to get reply for getCameraInfo command");
        }

        if (transport_->iface.size != 10) {
            /* we expect 10 bytes as answer */
            return Result<std::string>::error("Invalid response length for getCameraInfo command");
        }

        camera.vendor = (transport_->iface.ibuf[2] << 8) + transport_->iface.ibuf[3];
        camera.model = (transport_->iface.ibuf[4] << 8) + transport_->iface.ibuf[5];
        camera.rom_version = (transport_->iface.ibuf[6] << 8) + transport_->iface.ibuf[7];
        camera.socket_num = transport_->iface.ibuf[8];

        std::ostringstream oss;
        oss << getCameraVendor(camera.vendor) << ' ' << getCameraModel(camera.model)
            << ", ROM Version: 0x" << std::hex << std::uppercase << std::setfill('0') << std::setw(4)
            << static_cast<unsigned>(camera.rom_version)
            << ", Socket: 0x" << std::setw(2) << static_cast<unsigned>(camera.socket_num)
            << ", Address: 0x" << std::setw(2) << static_cast<unsigned>(camera.address)
            << std::dec << std::setfill(' ');

        return Result<std::string>::success(oss.str());
    }

    std::string_view Visca::getCameraVendor(const uint32_t vendor) {
        switch (static_cast<CameraVendors>(vendor)) {
            case CameraVendors::Sony:
                return "Sony";
            default:
                return "Unknown";
        }
    }

    std::string_view Visca::getCameraModel(const uint32_t model) {
        switch (static_cast<CameraModels>(model)) {
            case CameraModels::IX47X:
                return "IX47X";
            case CameraModels::EX47XL:
                return "EX47XL";
            case CameraModels::IX10:
                return "IX10";
            case CameraModels::EX780:
                return "EX780";
            case CameraModels::EX480A:
                return "EX480A";
            case CameraModels::EX480AP:
                return "EX480AP";
            case CameraModels::EX48Ax:
                return "EX48Ax";
            case CameraModels::EX45M:
                return "EX45M";
            case CameraModels::EX45MCE:
                return "EX45MCE";
            case CameraModels::IX47A:
                return "IX47A";
            case CameraModels::IX47AP:
                return "IX47AP";
            case CameraModels::IX45A:
                return "IX45A";
            case CameraModels::IX45AP:
                return "IX45AP";
            case CameraModels::IX10A:
                return "IX10A";
            case CameraModels::IX10AP:
                return "IX10AP";
            case CameraModels::EX780B:
                return "EX780B";
            case CameraModels::EX780BP:
                return "EX780BP";
            case CameraModels::EX78B:
                return "EX78B";
            case CameraModels::EX78BP:
                return "EX78BP";
            case CameraModels::EX480B:
                return "EX480B";
            case CameraModels::EX480BP:
                return "EX480BP";
            case CameraModels::EX48B:
                return "EX48B";
            case CameraModels::EX48BP:
                return "EX48BP";
            case CameraModels::EX980S:
                return "EX980S";
            case CameraModels::EX980SP:
                return "EX980SP";
            case CameraModels::EX980:
                return "EX980";
            case CameraModels::EX980P:
                return "EX980P";
            case CameraModels::EW9500H:
                return "EW9500H";
            case CameraModels::H10:
                return "H10";
            default:
                return "Unknown";
        }
    }

    /***********************************/
    /*       COMMAND FUNCTIONS         */
    /***********************************/

    ErrorCode Visca::setPower(const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_POWER);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setKeylock(const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_KEYLOCK);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setCameraId(const uint16_t id) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ID);
        appendByte(&packet, (id & 0xF000) >> 12);
        appendByte(&packet, (id & 0x0F00) >> 8);
        appendByte(&packet, (id & 0x00F0) >> 4);
        appendByte(&packet, (id & 0x000F));

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setZoomTele() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ZOOM);
        appendByte(&packet, VISCA_ZOOM_TELE);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setZoomWide() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ZOOM);
        appendByte(&packet, VISCA_ZOOM_WIDE);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setZoomStop() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ZOOM);
        appendByte(&packet, VISCA_ZOOM_STOP);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setZoomTeleSpeed(const uint32_t speed) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ZOOM);
        appendByte(&packet, VISCA_ZOOM_TELE_SPEED | (speed & 0x7));

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setZoomWideSpeed(const uint32_t speed) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ZOOM);
        appendByte(&packet, VISCA_ZOOM_WIDE_SPEED | (speed & 0x7));

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setZoomValue(const uint32_t zoom) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ZOOM_VALUE);
        appendByte(&packet, (zoom & 0xF000) >> 12);
        appendByte(&packet, (zoom & 0x0F00) >> 8);
        appendByte(&packet, (zoom & 0x00F0) >> 4);
        appendByte(&packet, (zoom & 0x000F));

        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return Result<void>::error(getViscaErrorMessage(err));
        }

        return Result<void>::success();
    }

    ErrorCode Visca::setZoomAndFocusValue(const uint32_t zoom, const uint32_t focus) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ZOOM_FOCUS_VALUE);
        appendByte(&packet, (zoom & 0xF000) >> 12);
        appendByte(&packet, (zoom & 0x0F00) >> 8);
        appendByte(&packet, (zoom & 0x00F0) >> 4);
        appendByte(&packet, (zoom & 0x000F));
        appendByte(&packet, (focus & 0xF000) >> 12);
        appendByte(&packet, (focus & 0x0F00) >> 8);
        appendByte(&packet, (focus & 0x00F0) >> 4);
        appendByte(&packet, (focus & 0x000F));

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setDzoom(const uint32_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DZOOM);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setDzoomLimit(const uint32_t limit) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DZOOM_LIMIT);
        appendByte(&packet, limit);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setDzoomMode(const uint32_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DZOOM_MODE);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setFocusFar() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS);
        appendByte(&packet, VISCA_FOCUS_FAR);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setFocusNear() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS);
        appendByte(&packet, VISCA_FOCUS_NEAR);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setFocusStop() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS);
        appendByte(&packet, VISCA_FOCUS_STOP);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setFocusFarSpeed(const uint32_t speed) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS);
        appendByte(&packet, VISCA_FOCUS_FAR_SPEED | (speed & 0x7));

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setFocusNearSpeed(const uint32_t speed) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS);
        appendByte(&packet, VISCA_FOCUS_NEAR_SPEED | (speed & 0x7));

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setFocusValue(const uint32_t focus) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_VALUE);
        appendByte(&packet, (focus & 0xF000) >> 12);
        appendByte(&packet, (focus & 0x0F00) >> 8);
        appendByte(&packet, (focus & 0x00F0) >> 4);
        appendByte(&packet, (focus & 0x000F));

        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return Result<void>::error(getViscaErrorMessage(err));
        }

        return Result<void>::success();
    }

    Result<void> Visca::setFocusAuto(const bool on) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_AUTO);
        appendByte(&packet, on ? VISCA_ON : VISCA_OFF);

        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return Result<void>::error(getViscaErrorMessage(err));
        }

        return Result<void>::success();
    }

    ErrorCode Visca::setFocusOnePush() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_ONE_PUSH);
        appendByte(&packet, VISCA_FOCUS_ONE_PUSH_TRIG);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setFocusInfinity() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_ONE_PUSH);
        appendByte(&packet, VISCA_FOCUS_ONE_PUSH_INF);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setFocusAutosenseHigh() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_AUTO_SENSE);
        appendByte(&packet, VISCA_FOCUS_AUTO_SENSE_HIGH);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setFocusAutosenseLow() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_AUTO_SENSE);
        appendByte(&packet, VISCA_FOCUS_AUTO_SENSE_LOW);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setFocusNearLimit(const uint32_t limit) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_NEAR_LIMIT);
        appendByte(&packet, (limit & 0xF000) >> 12);
        appendByte(&packet, (limit & 0x0F00) >> 8);
        appendByte(&packet, (limit & 0x00F0) >> 4);
        appendByte(&packet, (limit & 0x000F));

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setWhitebalMode(const uint32_t mode) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_WB);
        appendByte(&packet, mode);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setWhitebalOnePush() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_WB_TRIGGER);
        appendByte(&packet, VISCA_WB_ONE_PUSH_TRIG);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setRgainUp() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_RGAIN);
        appendByte(&packet, VISCA_UP);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setRgainDown() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_RGAIN);
        appendByte(&packet, VISCA_DOWN);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setRgainReset() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_RGAIN);
        appendByte(&packet, VISCA_RESET);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setRgainValue(const uint32_t value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_RGAIN_VALUE);
        appendByte(&packet, (value & 0xF000) >> 12);
        appendByte(&packet, (value & 0x0F00) >> 8);
        appendByte(&packet, (value & 0x00F0) >> 4);
        appendByte(&packet, (value & 0x000F));

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setBgainUp() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BGAIN);
        appendByte(&packet, VISCA_UP);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setBgainDown() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BGAIN);
        appendByte(&packet, VISCA_DOWN);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setBgainReset() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BGAIN);
        appendByte(&packet, VISCA_RESET);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setBgainValue(const uint32_t value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BGAIN_VALUE);
        appendByte(&packet, (value & 0xF000) >> 12);
        appendByte(&packet, (value & 0x0F00) >> 8);
        appendByte(&packet, (value & 0x00F0) >> 4);
        appendByte(&packet, (value & 0x000F));

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setShutterUp() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_SHUTTER);
        appendByte(&packet, VISCA_UP);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setShutterDown() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_SHUTTER);
        appendByte(&packet, VISCA_DOWN);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setShutterReset() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_SHUTTER);
        appendByte(&packet, VISCA_RESET);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setShutterValue(const uint32_t value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_SHUTTER_VALUE);
        appendByte(&packet, (value & 0xF000) >> 12);
        appendByte(&packet, (value & 0x0F00) >> 8);
        appendByte(&packet, (value & 0x00F0) >> 4);
        appendByte(&packet, (value & 0x000F));

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setIrisUp() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_IRIS);
        appendByte(&packet, VISCA_UP);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setIrisDown() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_IRIS);
        appendByte(&packet, VISCA_DOWN);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setIrisReset() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_IRIS);
        appendByte(&packet, VISCA_RESET);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setIrisValue(const uint32_t value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_IRIS_VALUE);
        appendByte(&packet, (value & 0xF000) >> 12);
        appendByte(&packet, (value & 0x0F00) >> 8);
        appendByte(&packet, (value & 0x00F0) >> 4);
        appendByte(&packet, (value & 0x000F));

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setGainUp() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_GAIN);
        appendByte(&packet, VISCA_UP);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setGainDown() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_GAIN);
        appendByte(&packet, VISCA_DOWN);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setGainReset() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_GAIN);
        appendByte(&packet, VISCA_RESET);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setGainValue(const uint32_t value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_GAIN_VALUE);
        appendByte(&packet, (value & 0xF000) >> 12);
        appendByte(&packet, (value & 0x0F00) >> 8);
        appendByte(&packet, (value & 0x00F0) >> 4);
        appendByte(&packet, (value & 0x000F));

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setBrightUp() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BRIGHT);
        appendByte(&packet, VISCA_UP);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setBrightDown() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BRIGHT);
        appendByte(&packet, VISCA_DOWN);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setBrightReset() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BRIGHT);
        appendByte(&packet, VISCA_RESET);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setBrightValue(const uint32_t value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BRIGHT_VALUE);
        appendByte(&packet, (value & 0xF000) >> 12);
        appendByte(&packet, (value & 0x0F00) >> 8);
        appendByte(&packet, (value & 0x00F0) >> 4);
        appendByte(&packet, (value & 0x000F));

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setApertureUp() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_APERTURE);
        appendByte(&packet, VISCA_UP);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setApertureDown() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_APERTURE);
        appendByte(&packet, VISCA_DOWN);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setApertureReset() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_APERTURE);
        appendByte(&packet, VISCA_RESET);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setApertureValue(const uint32_t value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_APERTURE_VALUE);
        appendByte(&packet, (value & 0xF000) >> 12);
        appendByte(&packet, (value & 0x0F00) >> 8);
        appendByte(&packet, (value & 0x00F0) >> 4);
        appendByte(&packet, (value & 0x000F));

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setExpCompUp() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_EXP_COMP);
        appendByte(&packet, VISCA_UP);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setExpCompDown() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_EXP_COMP);
        appendByte(&packet, VISCA_DOWN);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setExpCompReset() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_EXP_COMP);
        appendByte(&packet, VISCA_RESET);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setExpCompValue(const uint32_t value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_EXP_COMP_VALUE);
        appendByte(&packet, (value & 0xF000) >> 12);
        appendByte(&packet, (value & 0x0F00) >> 8);
        appendByte(&packet, (value & 0x00F0) >> 4);
        appendByte(&packet, (value & 0x000F));

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setExpCompPower(const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_EXP_COMP_POWER);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setAutoExpMode(const uint8_t mode) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_AUTO_EXP);
        appendByte(&packet, mode);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setSlowShutterAuto(const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_SLOW_SHUTTER);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setBacklightComp(const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BACKLIGHT_COMP);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setZeroLuxShot(const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ZERO_LUX);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setIrLed(const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_IR_LED);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setWideMode(const uint8_t mode) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_WIDE_MODE);
        appendByte(&packet, mode);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setMirror(const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_MIRROR);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setFreeze(const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FREEZE);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setPictureEffect(const uint8_t mode) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_PICTURE_EFFECT);
        appendByte(&packet, mode);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setDigitalEffect(const uint8_t mode) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DIGITAL_EFFECT);
        appendByte(&packet, mode);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setDigitalEffectLevel(const uint8_t level) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DIGITAL_EFFECT_LEVEL);
        appendByte(&packet, level);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setCamStabilizer(const bool power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_CAM_STABILIZER);

        if (power) {
            appendByte(&packet, VISCA_ON);
        } else {
            appendByte(&packet, VISCA_OFF);
        }

        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return Result<void>::error(getViscaErrorMessage(err));
        }

        return Result<void>::success();
    }

    ErrorCode Visca::memorySet(const uint8_t channel) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_MEMORY);
        appendByte(&packet, VISCA_MEMORY_SET);
        appendByte(&packet, channel);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::memoryRecall(const uint8_t channel) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_MEMORY);
        appendByte(&packet, VISCA_MEMORY_RECALL);
        appendByte(&packet, channel);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::memoryReset(const uint8_t channel) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_MEMORY);
        appendByte(&packet, VISCA_MEMORY_RESET);
        appendByte(&packet, channel);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setDisplay(const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DISPLAY);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setDateTime(const uint32_t year, const uint32_t month, const uint32_t day, const uint32_t hour,
                                 const uint32_t minute) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DATE_TIME_SET);
        appendByte(&packet, year / 10);
        appendByte(&packet, year - 10 * (year / 10));
        appendByte(&packet, month / 10);
        appendByte(&packet, month - 10 * (month / 10));
        appendByte(&packet, day / 10);
        appendByte(&packet, day - 10 * (day / 10));
        appendByte(&packet, hour / 10);
        appendByte(&packet, hour - 10 * (hour / 10));
        appendByte(&packet, minute / 10);
        appendByte(&packet, minute - 10 * (minute / 10));

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setDateDisplay(const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DATE_DISPLAY);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setTimeDisplay(const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_TIME_DISPLAY);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setTitleDisplay(const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_TITLE_DISPLAY);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setTitleClear() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_TITLE_DISPLAY);
        appendByte(&packet, VISCA_TITLE_DISPLAY_CLEAR);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setTitleParams(const ViscaTitleData* title) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_TITLE_SET);
        appendByte(&packet, VISCA_TITLE_SET_PARAMS);
        appendByte(&packet, title->vposition);
        appendByte(&packet, title->hposition);
        appendByte(&packet, title->color);
        appendByte(&packet, title->blink);
        appendByte(&packet, 0);
        appendByte(&packet, 0);
        appendByte(&packet, 0);
        appendByte(&packet, 0);
        appendByte(&packet, 0);
        appendByte(&packet, 0);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setTitle(const ViscaTitleData* title) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_TITLE_SET);
        appendByte(&packet, VISCA_TITLE_SET_PART1);

        for (auto i = 0; i < 10; i++) {
            appendByte(&packet, title->title[i]);
        }

        if (sendPacketWithReply(&packet) != ErrorCode::Success) {
            return ErrorCode::Failure;
        }

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_TITLE_SET);
        appendByte(&packet, VISCA_TITLE_SET_PART2);

        for (auto i = 0; i < 10; i++) {
            appendByte(&packet, title->title[i + 10]);
        }

        if (sendPacketWithReply(&packet) != ErrorCode::Success) {
            return ErrorCode::Failure;
        }

        return ErrorCode::Success;
    }

    ErrorCode Visca::setSpotAeOn() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_SPOT_AE);
        appendByte(&packet, VISCA_ON);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setSpotAeOff() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_SPOT_AE);
        appendByte(&packet, VISCA_OFF);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setSpotAePosition(const uint8_t x_position, const uint8_t y_position) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_SPOT_AE_POSITION);
        appendByte(&packet, (x_position & 0xF0) >> 4);
        appendByte(&packet, (x_position & 0x0F));
        appendByte(&packet, (y_position & 0xF0) >> 4);
        appendByte(&packet, (y_position & 0x0F));

        return sendPacketWithReply(&packet);
    }

    /***********************************/
    /*       INQUIRY FUNCTIONS         */
    /***********************************/

    ErrorCode Visca::getPower(uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_POWER);
        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *power = transport_->iface.ibuf[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getDzoom(uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DZOOM);
        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *power = transport_->iface.ibuf[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getDzoomLimit(uint8_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DZOOM_LIMIT);
        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *value = transport_->iface.ibuf[2];
        return ErrorCode::Success;
    }

    Result<uint16_t> Visca::getZoomValue() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ZOOM_VALUE);

        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return Result<uint16_t>::error(getViscaErrorMessage(err));
        }
        const uint16_t value = (transport_->iface.ibuf[2] << 12) + (transport_->iface.ibuf[3] << 8) + (transport_->iface.ibuf[
            4] << 4) + transport_->iface.ibuf[5];
        return Result<uint16_t>::success(value);
    }

    Result<bool> Visca::getFocusAuto() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_AUTO);

        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return Result<bool>::error(getViscaErrorMessage(err));
        }

        if (transport_->iface.ibuf[2] == VISCA_OFF) {
            return Result<bool>::success(false);
        }

        return Result<bool>::success(true);
    }

    Result<uint16_t> Visca::getFocusValue() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_VALUE);

        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return Result<uint16_t>::error(getViscaErrorMessage(err));
        }

        const uint16_t value = (transport_->iface.ibuf[2] << 12) + (transport_->iface.ibuf[3] << 8) + (transport_->iface
            .ibuf[4] << 4) + transport_->iface.ibuf[5];
        return Result<uint16_t>::success(value);
    }

    ErrorCode Visca::getFocusAutoSense(uint8_t* mode) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_AUTO_SENSE);

        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *mode = transport_->iface.ibuf[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getFocusNearLimit(uint16_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_NEAR_LIMIT);

        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *value = (transport_->iface.ibuf[2] << 12) + (transport_->iface.ibuf[3] << 8) + (transport_->iface.ibuf[4] << 4)
            + transport_->iface.ibuf[5];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getWhitebalMode(uint8_t* mode) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_WB);

        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *mode = transport_->iface.ibuf[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getRgainValue(uint16_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_RGAIN_VALUE);

        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *value = (transport_->iface.ibuf[2] << 12) + (transport_->iface.ibuf[3] << 8) + (transport_->iface.ibuf[4] << 4)
            + transport_->iface.ibuf[5];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getBgainValue(uint16_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BGAIN_VALUE);

        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *value = (transport_->iface.ibuf[2] << 12) + (transport_->iface.ibuf[3] << 8) + (transport_->iface.ibuf[4] << 4)
            + transport_->iface.ibuf[5];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getAutoExpMode(uint8_t* mode) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_AUTO_EXP);

        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *mode = transport_->iface.ibuf[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getSlowShutterAuto(uint8_t* mode) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_SLOW_SHUTTER);

        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *mode = transport_->iface.ibuf[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getShutterValue(uint16_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_SHUTTER_VALUE);

        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *value = (transport_->iface.ibuf[2] << 12) + (transport_->iface.ibuf[3] << 8) + (transport_->iface.ibuf[4] << 4)
            + transport_->iface.ibuf[5];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getIrisValue(uint16_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_IRIS_VALUE);

        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *value = (transport_->iface.ibuf[2] << 12) + (transport_->iface.ibuf[3] << 8) + (transport_->iface.ibuf[4] << 4)
            + transport_->iface.ibuf[5];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getGainValue(uint16_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_GAIN_VALUE);

        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *value = (transport_->iface.ibuf[2] << 12) + (transport_->iface.ibuf[3] << 8) + (transport_->iface.ibuf[4] << 4)
            + transport_->iface.ibuf[5];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getBrightValue(uint16_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BRIGHT_VALUE);

        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *value = (transport_->iface.ibuf[2] << 12) + (transport_->iface.ibuf[3] << 8) + (transport_->iface.ibuf[4] << 4)
            + transport_->iface.ibuf[5];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getExpCompPower(uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_EXP_COMP_POWER);

        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *power = transport_->iface.ibuf[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getExpCompValue(uint16_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_EXP_COMP_VALUE);

        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *value = (transport_->iface.ibuf[2] << 12) + (transport_->iface.ibuf[3] << 8) + (transport_->iface.ibuf[4] << 4)
            + transport_->iface.ibuf[5];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getBacklightComp(uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BACKLIGHT_COMP);

        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *power = transport_->iface.ibuf[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getApertureValue(uint16_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_APERTURE_VALUE);

        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *value = (transport_->iface.ibuf[2] << 12) + (transport_->iface.ibuf[3] << 8) + (transport_->iface.ibuf[4] << 4)
            + transport_->iface.ibuf[5];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getZeroLuxShot(uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ZERO_LUX);

        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *power = transport_->iface.ibuf[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getIrLed(uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_IR_LED);

        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *power = transport_->iface.ibuf[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getWideMode(uint8_t* mode) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_WIDE_MODE);

        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *mode = transport_->iface.ibuf[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getMirror(uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_MIRROR);

        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *power = transport_->iface.ibuf[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getFreeze(uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FREEZE);

        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *power = transport_->iface.ibuf[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getPictureEffect(uint8_t* mode) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_PICTURE_EFFECT);

        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *mode = transport_->iface.ibuf[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getDigitalEffect(uint8_t* mode) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DIGITAL_EFFECT);

        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *mode = transport_->iface.ibuf[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getDigitalEffectLevel(uint16_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DIGITAL_EFFECT_LEVEL);

        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *value = (transport_->iface.ibuf[2] << 12) + (transport_->iface.ibuf[3] << 8) + (transport_->iface.ibuf[4] << 4)
            + transport_->iface.ibuf[5];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getMemory(uint8_t* channel) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_MEMORY);

        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *channel = transport_->iface.ibuf[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getDisplay(uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DISPLAY);

        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *power = transport_->iface.ibuf[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getId(uint16_t* id) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ID);

        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *id = (transport_->iface.ibuf[2] << 12) + (transport_->iface.ibuf[3] << 8) + (transport_->iface.ibuf[4] << 4) +
            transport_->iface.ibuf[5];
        return ErrorCode::Success;
    }

    ErrorCode Visca::setIrreceiveOn() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_IRRECEIVE);
        appendByte(&packet, VISCA_ON);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setIrreceiveOff() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_IRRECEIVE);
        appendByte(&packet, VISCA_OFF);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setIrreceiveOnoff() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_IRRECEIVE);
        appendByte(&packet, VISCA_IRRECEIVE_ONOFF);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setPantiltUp(const uint32_t pan_speed, const uint32_t tilt_speed) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_DRIVE);
        appendByte(&packet, pan_speed);
        appendByte(&packet, tilt_speed);
        appendByte(&packet, VISCA_PT_DRIVE_HORIZ_STOP);
        appendByte(&packet, VISCA_PT_DRIVE_VERT_UP);
        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setPantiltDown(const uint32_t pan_speed, const uint32_t tilt_speed) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_DRIVE);
        appendByte(&packet, pan_speed);
        appendByte(&packet, tilt_speed);
        appendByte(&packet, VISCA_PT_DRIVE_HORIZ_STOP);
        appendByte(&packet, VISCA_PT_DRIVE_VERT_DOWN);
        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setPantiltLeft(const uint32_t pan_speed, const uint32_t tilt_speed) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_DRIVE);
        appendByte(&packet, pan_speed);
        appendByte(&packet, tilt_speed);
        appendByte(&packet, VISCA_PT_DRIVE_HORIZ_LEFT);
        appendByte(&packet, VISCA_PT_DRIVE_VERT_STOP);
        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setPantiltRight(const uint32_t pan_speed, const uint32_t tilt_speed) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_DRIVE);
        appendByte(&packet, pan_speed);
        appendByte(&packet, tilt_speed);
        appendByte(&packet, VISCA_PT_DRIVE_HORIZ_RIGHT);
        appendByte(&packet, VISCA_PT_DRIVE_VERT_STOP);
        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setPantiltUpleft(const uint32_t pan_speed, const uint32_t tilt_speed) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_DRIVE);
        appendByte(&packet, pan_speed);
        appendByte(&packet, tilt_speed);
        appendByte(&packet, VISCA_PT_DRIVE_HORIZ_LEFT);
        appendByte(&packet, VISCA_PT_DRIVE_VERT_UP);
        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setPantiltUpright(const uint32_t pan_speed, const uint32_t tilt_speed) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_DRIVE);
        appendByte(&packet, pan_speed);
        appendByte(&packet, tilt_speed);
        appendByte(&packet, VISCA_PT_DRIVE_HORIZ_RIGHT);
        appendByte(&packet, VISCA_PT_DRIVE_VERT_UP);
        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setPantiltDownleft(const uint32_t pan_speed, const uint32_t tilt_speed) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_DRIVE);
        appendByte(&packet, pan_speed);
        appendByte(&packet, tilt_speed);
        appendByte(&packet, VISCA_PT_DRIVE_HORIZ_LEFT);
        appendByte(&packet, VISCA_PT_DRIVE_VERT_DOWN);
        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setPantiltDownright(const uint32_t pan_speed, const uint32_t tilt_speed) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_DRIVE);
        appendByte(&packet, pan_speed);
        appendByte(&packet, tilt_speed);
        appendByte(&packet, VISCA_PT_DRIVE_HORIZ_RIGHT);
        appendByte(&packet, VISCA_PT_DRIVE_VERT_DOWN);
        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setPantiltStop(const uint32_t pan_speed, const uint32_t tilt_speed) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_DRIVE);
        appendByte(&packet, pan_speed);
        appendByte(&packet, tilt_speed);
        appendByte(&packet, VISCA_PT_DRIVE_HORIZ_STOP);
        appendByte(&packet, VISCA_PT_DRIVE_VERT_STOP);
        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setPantiltAbsolutePosition(const uint32_t pan_speed, const uint32_t tilt_speed,
                                                const uint32_t pan_pos, const uint32_t tilt_pos) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_ABSOLUTE_POSITION);
        appendByte(&packet, pan_speed);
        appendByte(&packet, tilt_speed);

        appendByte(&packet, (pan_pos & 0x0f000) >> 12);
        appendByte(&packet, (pan_pos & 0x00f00) >> 8);
        appendByte(&packet, (pan_pos & 0x000f0) >> 4);
        appendByte(&packet, pan_pos & 0x0000f);

        appendByte(&packet, (tilt_pos & 0xf000) >> 12);
        appendByte(&packet, (tilt_pos & 0x0f00) >> 8);
        appendByte(&packet, (tilt_pos & 0x00f0) >> 4);
        appendByte(&packet, tilt_pos & 0x000f);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setPantiltRelativePosition(const uint32_t pan_speed, const uint32_t tilt_speed,
                                                const uint32_t pan_pos, const uint32_t tilt_pos) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_RELATIVE_POSITION);
        appendByte(&packet, pan_speed);
        appendByte(&packet, tilt_speed);

        appendByte(&packet, (pan_pos & 0x0f000) >> 12);
        appendByte(&packet, (pan_pos & 0x00f00) >> 8);
        appendByte(&packet, (pan_pos & 0x000f0) >> 4);
        appendByte(&packet, pan_pos & 0x0000f);

        appendByte(&packet, (tilt_pos & 0xf000) >> 12);
        appendByte(&packet, (tilt_pos & 0x0f00) >> 8);
        appendByte(&packet, (tilt_pos & 0x00f0) >> 4);
        appendByte(&packet, tilt_pos & 0x000f);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setPantiltHome() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_HOME);
        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setPantiltReset() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_RESET);
        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setPantiltLimitUpright(const uint32_t pan_pos, const uint32_t tilt_pos) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_LIMITSET);
        appendByte(&packet, VISCA_PT_LIMITSET_SET);
        appendByte(&packet, VISCA_PT_LIMITSET_SET_UR);
        appendByte(&packet, (pan_pos & 0xf000) >> 12);
        appendByte(&packet, (pan_pos & 0x0f00) >> 8);
        appendByte(&packet, (pan_pos & 0x00f0) >> 4);
        appendByte(&packet, pan_pos & 0x000f);

        appendByte(&packet, (tilt_pos & 0xf000) >> 12);
        appendByte(&packet, (tilt_pos & 0x0f00) >> 8);
        appendByte(&packet, (tilt_pos & 0x00f0) >> 4);
        appendByte(&packet, tilt_pos & 0x000f);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setPantiltLimitDownleft(const uint32_t pan_pos, const uint32_t tilt_pos) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_LIMITSET);
        appendByte(&packet, VISCA_PT_LIMITSET_SET);
        appendByte(&packet, VISCA_PT_LIMITSET_SET_DL);
        appendByte(&packet, (pan_pos & 0xf000) >> 12);
        appendByte(&packet, (pan_pos & 0x0f00) >> 8);
        appendByte(&packet, (pan_pos & 0x00f0) >> 4);
        appendByte(&packet, pan_pos & 0x000f);

        appendByte(&packet, (tilt_pos & 0xf000) >> 12);
        appendByte(&packet, (tilt_pos & 0x0f00) >> 8);
        appendByte(&packet, (tilt_pos & 0x00f0) >> 4);
        appendByte(&packet, tilt_pos & 0x000f);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setPantiltLimitDownleftClear() {
        ViscaPacket packet{};

        constexpr uint32_t pan_pos = 0x7fff;
        constexpr uint32_t tilt_pos = 0x7fff;

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_LIMITSET);
        appendByte(&packet, VISCA_PT_LIMITSET_CLEAR);
        appendByte(&packet, VISCA_PT_LIMITSET_SET_DL);
        appendByte(&packet, (pan_pos & 0xf000) >> 12);
        appendByte(&packet, (pan_pos & 0x0f00) >> 8);
        appendByte(&packet, (pan_pos & 0x00f0) >> 4);
        appendByte(&packet, pan_pos & 0x000f);

        appendByte(&packet, (tilt_pos & 0xf000) >> 12);
        appendByte(&packet, (tilt_pos & 0x0f00) >> 8);
        appendByte(&packet, (tilt_pos & 0x00f0) >> 4);
        appendByte(&packet, tilt_pos & 0x000f);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setPantiltLimitUprightClear() {
        ViscaPacket packet{};

        constexpr uint32_t pan_pos = 0x7fff;
        constexpr uint32_t tilt_pos = 0x7fff;

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_LIMITSET);
        appendByte(&packet, VISCA_PT_LIMITSET_CLEAR);
        appendByte(&packet, VISCA_PT_LIMITSET_SET_UR);
        appendByte(&packet, (pan_pos & 0xf000) >> 12);
        appendByte(&packet, (pan_pos & 0x0f00) >> 8);
        appendByte(&packet, (pan_pos & 0x00f0) >> 4);
        appendByte(&packet, pan_pos & 0x000f);

        appendByte(&packet, (tilt_pos & 0xf000) >> 12);
        appendByte(&packet, (tilt_pos & 0x0f00) >> 8);
        appendByte(&packet, (tilt_pos & 0x00f0) >> 4);
        appendByte(&packet, tilt_pos & 0x000f);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setDatascreenOn() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_DATASCREEN);
        appendByte(&packet, VISCA_ON);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setDatascreenOff() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_DATASCREEN);
        appendByte(&packet, VISCA_OFF);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setDatascreenOnoff() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_DATASCREEN);
        appendByte(&packet, VISCA_PT_DATASCREEN_ONOFF);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setRegister(const uint8_t reg_num, const uint8_t reg_val) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_REGISTER_VALUE);
        appendByte(&packet, reg_num);
        appendByte(&packet, (reg_val & 0xF0) >> 4);
        appendByte(&packet, (reg_val & 0x0F));
        return sendPacketWithReply(&packet);
    }

    /***********************************/
    /*       INQUIRY FUNCTIONS         */
    /***********************************/

    ErrorCode Visca::getVideosystem(uint8_t* system) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_VIDEOSYSTEM_INQ);

        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *system = transport_->iface.ibuf[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getPantiltMode(uint16_t* status) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_MODE_INQ);

        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *status = ((transport_->iface.ibuf[2] & 0xff) << 8) + (transport_->iface.ibuf[3] & 0xff);
        return ErrorCode::Success;
    }

    ErrorCode Visca::getPantiltMaxspeed(uint8_t* max_pan_speed, uint8_t* max_tilt_speed) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_MAXSPEED_INQ);

        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *max_pan_speed = (transport_->iface.ibuf[2] & 0xff);
        *max_tilt_speed = (transport_->iface.ibuf[3] & 0xff);
        return ErrorCode::Success;
    }

    ErrorCode Visca::getPantiltPosition(uint16_t* pan_position, uint16_t* tilt_position) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_POSITION_INQ);

        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *pan_position = ((transport_->iface.ibuf[2] & 0xf) << 12) + ((transport_->iface.ibuf[3] & 0xf) << 8) + ((
            transport_->iface.ibuf[4] & 0xf) << 4) + (transport_->iface.ibuf[5] & 0xf);
        *tilt_position = ((transport_->iface.ibuf[6] & 0xf) << 12) + ((transport_->iface.ibuf[7] & 0xf) << 8) + ((
            transport_->iface.ibuf[8] & 0xf) << 4) + (transport_->iface.ibuf[9] & 0xf);

        return ErrorCode::Success;
    }

    ErrorCode Visca::getDatascreen(uint8_t* status) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_DATASCREEN_INQ);

        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *status = transport_->iface.ibuf[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getRegister(const uint8_t reg_num, uint8_t* reg_val) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_REGISTER_VALUE);
        appendByte(&packet, reg_num);

        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *reg_val = (transport_->iface.ibuf[2] << 4) + transport_->iface.ibuf[3];
        return ErrorCode::Success;
    }

    /********************************/
    /* SPECIAL FUNCTIONS FOR D30/31 */
    /********************************/

    ErrorCode Visca::setWideConLens(const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_WIDE_CON_LENS);
        appendByte(&packet, VISCA_WIDE_CON_LENS_SET);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setAtModeOnoff() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_MODE);
        appendByte(&packet, VISCA_AT_ONOFF);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setAtMode(const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_MODE);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setAtAeOnoff() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_AE);
        appendByte(&packet, VISCA_AT_ONOFF);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setAtAe(const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_AE);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setAtAutozoomOnoff() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_AUTOZOOM);
        appendByte(&packet, VISCA_AT_ONOFF);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setAtAutozoom(const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_AUTOZOOM);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setAtmdFramedisplayOnoff() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_ATMD_FRAMEDISPLAY);
        appendByte(&packet, VISCA_AT_ONOFF);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setAtmdFramedisplay(const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_ATMD_FRAMEDISPLAY);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setAtFrameoffsetOnoff() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_FRAMEOFFSET);
        appendByte(&packet, VISCA_AT_ONOFF);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setAtFrameoffset(const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_FRAMEOFFSET);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setAtmdStartstop() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_ATMD_STARTSTOP);
        appendByte(&packet, VISCA_AT_ONOFF);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setAtChase(const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_CHASE);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setAtChaseNext() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_CHASE);
        appendByte(&packet, VISCA_AT_CHASE_NEXT);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setMdModeOnoff() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_MODE);
        appendByte(&packet, VISCA_MD_ONOFF);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setMdMode(const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_MODE);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setMdFrame() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_FRAME);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setMdDetect() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_DETECT);
        appendByte(&packet, VISCA_MD_ONOFF);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setAtEntry(const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_ENTRY);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setAtLostinfo() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_ATMD_LOSTINFO1);
        appendByte(&packet, VISCA_ATMD_LOSTINFO2);
        appendByte(&packet, VISCA_AT_LOSTINFO);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setMdLostinfo() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_ATMD_LOSTINFO1);
        appendByte(&packet, VISCA_ATMD_LOSTINFO2);
        appendByte(&packet, VISCA_MD_LOSTINFO);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setMdAdjustYlevel(const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_YLEVEL);
        appendByte(&packet, VISCA_MD_ADJUST);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setMdAdjustHuelevel(const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_HUELEVEL);
        appendByte(&packet, VISCA_MD_ADJUST);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setMdAdjustSize(const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_SIZE);
        appendByte(&packet, VISCA_MD_ADJUST);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setMdAdjustDisptime(const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_DISPTIME);
        appendByte(&packet, VISCA_MD_ADJUST);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setMdAdjustRefmode(const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_REFMODE);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setMdAdjustReftime(const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_REFTIME);
        appendByte(&packet, VISCA_MD_ADJUST);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setMdMeasureMode1Onoff() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_MEASURE_MODE_1);
        appendByte(&packet, VISCA_MD_ONOFF);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setMdMeasureMode1(const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_MEASURE_MODE_1);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setMdMeasureMode2Onoff() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_MEASURE_MODE_2);
        appendByte(&packet, VISCA_MD_ONOFF);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setMdMeasureMode2(const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_MEASURE_MODE_2);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::getKeylock(uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_KEYLOCK);

        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *power = transport_->iface.ibuf[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getWideConLens(uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_WIDE_CON_LENS);

        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *power = transport_->iface.ibuf[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getAtmdMode(uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_ATMD_MODE);

        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *power = transport_->iface.ibuf[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getAtMode(uint16_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_MODE_QUERY);

        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *value = ((transport_->iface.ibuf[2] & 0xff) << 8) + (transport_->iface.ibuf[3] & 0xff);
        return ErrorCode::Success;
    }

    ErrorCode Visca::getAtEntry(uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_ENTRY);

        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *power = transport_->iface.ibuf[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getMdMode(uint16_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_MODE_QUERY);
        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *value = ((transport_->iface.ibuf[2] & 0xff) << 8) + (transport_->iface.ibuf[3] & 0xff);
        return ErrorCode::Success;
    }

    ErrorCode Visca::getMdYlevel(uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_YLEVEL);
        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *power = (transport_->iface.ibuf[3] & 0x0f);
        return ErrorCode::Success;
    }

    ErrorCode Visca::getMdHuelevel(uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_HUELEVEL);
        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *power = (transport_->iface.ibuf[3] & 0x0f);
        return ErrorCode::Success;
    }

    ErrorCode Visca::getMdSize(uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_SIZE);
        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *power = (transport_->iface.ibuf[3] & 0x0f);
        return ErrorCode::Success;
    }

    ErrorCode Visca::getMdDisptime(uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_DISPTIME);

        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *power = (transport_->iface.ibuf[3] & 0x0f);
        return ErrorCode::Success;
    }

    ErrorCode Visca::getMdRefmode(uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_REFMODE);

        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *power = transport_->iface.ibuf[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getMdReftime(uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_REFTIME_QUERY);

        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *power = (transport_->iface.ibuf[3] & 0x0f);
        return ErrorCode::Success;
    }

    ErrorCode Visca::getAtObjPos(uint8_t* xpos, uint8_t* ypos, uint8_t* status) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_POSITION);

        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *xpos = transport_->iface.ibuf[2];
        *ypos = transport_->iface.ibuf[3];
        *status = (transport_->iface.ibuf[4] & 0x0f);
        return ErrorCode::Success;
    }

    ErrorCode Visca::getMdObjPos(uint8_t* xpos, uint8_t* ypos, uint8_t* status) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_POSITION);

        if (const auto err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *xpos = transport_->iface.ibuf[2];
        *ypos = transport_->iface.ibuf[3];
        *status = (transport_->iface.ibuf[4] & 0x0f);
        return ErrorCode::Success;
    }

    std::string Visca::getViscaErrorMessage(const ErrorCode error_code) {
        switch (error_code) {
            case ErrorCode::ErrorMessageLength:
                return "Invalid message length";
            case ErrorCode::ErrorSyntax:
                return "Syntax error";
            case ErrorCode::ErrorCmdBufferFull:
                return "Command buffer full";
            case ErrorCode::ErrorCmdCancelled:
                return "Command cancelled";
            case ErrorCode::ErrorNoSocket:
                return "No socket available";
            case ErrorCode::ErrorCmdNotExecutable:
                return "Command not executable";
            default:
                return "Unknown VISCA error: " + std::to_string(static_cast<uint32_t>(error_code));
        }
    }
}