#include "Visca.h"

#include <algorithm>
#include <unistd.h>
#include <sys/ioctl.h>

namespace camera_service::infrastructure {
    Visca::Visca(std::unique_ptr<Uart> transport) : transport_(std::move(transport)) {
    }

    ResultCode Visca::write(const ViscaPacket* packet) const {
        if (const auto result = transport_->write(
                std::span<const std::byte>(reinterpret_cast<const std::byte*>(packet->bytes), packet->size)); result.
            isError()) {
            return ResultCode::Failure;
        }
        return ResultCode::Success;
    }

    ResultCode Visca::sendPacket(ViscaPacket* packet) const {
        // check data:
        if ((transport_->iface.address > 7) || (camera.address > 7) || (transport_->iface.broadcast > 1)) {
            return ResultCode::Failure;
        }

        // build header:
        packet->bytes[0] = VISCA_START_BYTE;
        packet->bytes[0] |= (transport_->iface.address << 4);
        if (transport_->iface.broadcast > 0) {
            packet->bytes[0] |= (transport_->iface.broadcast << 3);
            packet->bytes[0] &= 0xF8;
        }
        else {
            packet->bytes[0] |= camera.address;
        }

        // append footer
        appendByte(packet, VISCA_TERMINATOR);

        return write(packet);
    }

    ResultCode Visca::read() const {
        auto result = transport_->read();
        if (result.isError()) {
            return ResultCode::Failure;
        }

        auto data = std::move(result).value();
        if (data.empty()) {
            return ResultCode::Failure;
        }

        // Find the terminator in the received data
        const auto terminator_it = std::ranges::find(data, static_cast<std::byte>(VISCA_TERMINATOR));
        if (terminator_it == data.end()) {
            // No terminator found, this shouldn't happen in a valid VISCA message
            return ResultCode::Failure;
        }

        // Calculate the number of bytes including the terminator
        transport_->iface.size = std::distance(data.begin(), terminator_it) + 1;

        // Copy data to ibuf
        for (size_t i = 0; i < transport_->iface.size && i < sizeof(transport_->iface.ibuf); ++i) {
            transport_->iface.ibuf[i] = static_cast<uint8_t>(data[i]);
        }

        return ResultCode::Success;
    }

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

    void Visca::appendAsNibbles(ViscaPacket* packet, const uint16_t value) {
        appendByte(packet, (value & 0xF000) >> 12);
        appendByte(packet, (value & 0x0F00) >> 8);
        appendByte(packet, (value & 0x00F0) >> 4);
        appendByte(packet, (value & 0x000F));
    }

    ResultCode Visca::getReply() const {
        if (read() != ResultCode::Success) { // Read first message
            return ResultCode::Failure;
        }
        transport_->iface.type = static_cast<ResponseType>(transport_->iface.ibuf[1] & 0xF0);

        while (transport_->iface.type == ResponseType::Ack) { // Skip ack messages
            if (read() != ResultCode::Success) { // Read second message
                return ResultCode::Failure;
            }
            transport_->iface.type = static_cast<ResponseType>(transport_->iface.ibuf[1] & 0xF0);
        }

        switch (transport_->iface.type) {
            case ResponseType::Clear:
            case ResponseType::Address:
            case ResponseType::Completed:
            case ResponseType::Error:
                return ResultCode::Success;
                break;
            default:
                return ResultCode::Failure;
        }
    }

    ResultCode Visca::sendPacketWithReply(ViscaPacket* packet) const {
        if (sendPacket(packet) != ResultCode::Success) {
            return ResultCode::Failure;
        }

        if (getReply() != ResultCode::Success) {
            return ResultCode::Failure;
        }

        if (transport_->iface.type == ResponseType::Error) {
            return static_cast<ResultCode>(transport_->iface.ibuf[2]);
        }

        return ResultCode::Success;
    }

    ResultCode Visca::unreadBytes(const uint8_t* buffer, size_t* buffer_size) const {
        size_t bytes = 0;
        *buffer_size = 0;

        ioctl(transport_->iface.port_fd, FIONREAD, &bytes);
        if (bytes > 0) {
            bytes = (bytes > *buffer_size) ? *buffer_size : bytes;
            ::read(transport_->iface.port_fd, &buffer, bytes);
            *buffer_size = bytes;
            return ResultCode::Failure;
        }
        return ResultCode::Success;
    }

    Result<void> Visca::setAddress() {
        ViscaPacket packet{};
        int32_t camera_num = 0;

        const auto backup = transport_->iface.broadcast;

        appendByte(&packet, 0x30);
        appendByte(&packet, 0x01);

        transport_->iface.broadcast = 1;
        if (sendPacket(&packet) != ResultCode::Success) {
            transport_->iface.broadcast = backup;
            return Result<void>::error("Failed to send setAddress command");
        }
        transport_->iface.broadcast = backup;

        if (getReply() != ResultCode::Success) {
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

        appendByte(&packet, 0x01);
        appendByte(&packet, 0x00);
        appendByte(&packet, 0x01);

        if (sendPacket(&packet) != ResultCode::Success) {
            return Result<void>::error("Failed to send clear command");
        }
        if (getReply() != ResultCode::Success) {
            return Result<void>::error("Failed to get reply for clear command");
        }
        return Result<void>::success();
    }

    std::string_view Visca::getCameraVendor(const uint16_t vendor) {
        switch (static_cast<CameraVendors>(vendor)) {
            case CameraVendors::Sony:
                return "Sony";
            default:
                return "Unknown";
        }
    }

    std::string_view Visca::getCameraModel(const uint16_t model) {
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

    ResultCode Visca::setPower(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_POWER);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setKeylock(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_KEYLOCK);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setCameraId(const uint16_t id) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ID);
        appendAsNibbles(&packet, id);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setZoomTele() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ZOOM);
        appendByte(&packet, VISCA_ZOOM_TELE);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setZoomWide() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ZOOM);
        appendByte(&packet, VISCA_ZOOM_WIDE);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setZoomStop() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ZOOM);
        appendByte(&packet, VISCA_ZOOM_STOP);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setZoomTeleSpeed(const uint32_t speed) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ZOOM);
        appendByte(&packet, VISCA_ZOOM_TELE_SPEED | (speed & 0x7));

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setZoomWideSpeed(const uint32_t speed) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ZOOM);
        appendByte(&packet, VISCA_ZOOM_WIDE_SPEED | (speed & 0x7));

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setZoomValue(const uint16_t zoom) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ZOOM_VALUE);
        appendAsNibbles(&packet, zoom);

        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return Result<void>::error(getViscaErrorMessage(err));
        }

        return Result<void>::success();
    }

    ResultCode Visca::setZoomAndFocusValue(const uint16_t zoom, const uint16_t focus) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ZOOM_FOCUS_VALUE);
        appendAsNibbles(&packet, zoom);
        appendAsNibbles(&packet, focus);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setDzoomValue(const uint8_t value) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DZOOM_VALUE);
        appendAsNibbles(&packet, value);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setDzoomLimit(const uint8_t limit) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DZOOM_LIMIT);
        appendByte(&packet, limit);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setDzoomMode(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DZOOM_MODE);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setFocusFar() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS);
        appendByte(&packet, VISCA_FOCUS_FAR);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setFocusNear() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS);
        appendByte(&packet, VISCA_FOCUS_NEAR);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setFocusStop() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS);
        appendByte(&packet, VISCA_FOCUS_STOP);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setFocusFarSpeed(const uint32_t speed) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS);
        appendByte(&packet, VISCA_FOCUS_FAR_SPEED | (speed & 0x7));

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setFocusNearSpeed(const uint32_t speed) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS);
        appendByte(&packet, VISCA_FOCUS_NEAR_SPEED | (speed & 0x7));

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setFocusValue(const uint16_t focus) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_VALUE);
        appendAsNibbles(&packet, focus);

        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return Result<void>::error(getViscaErrorMessage(err));
        }

        return Result<void>::success();
    }

    Result<void> Visca::setFocusAuto(const bool on) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_AUTO);
        appendByte(&packet, on ? VISCA_ON : VISCA_OFF);

        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return Result<void>::error(getViscaErrorMessage(err));
        }

        return Result<void>::success();
    }

    ResultCode Visca::setFocusOnePush() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_ONE_PUSH);
        appendByte(&packet, VISCA_FOCUS_ONE_PUSH_TRIG);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setFocusInfinity() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_ONE_PUSH);
        appendByte(&packet, VISCA_FOCUS_ONE_PUSH_INF);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setFocusAutosenseHigh() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_AUTO_SENSE);
        appendByte(&packet, VISCA_FOCUS_AUTO_SENSE_HIGH);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setFocusAutosenseLow() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_AUTO_SENSE);
        appendByte(&packet, VISCA_FOCUS_AUTO_SENSE_LOW);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setFocusNearLimit(const uint16_t limit) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_NEAR_LIMIT);
        appendAsNibbles(&packet, limit);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setWhitebalMode(const uint8_t mode) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_WB);
        appendByte(&packet, mode);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setWhitebalOnePush() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_WB_TRIGGER);
        appendByte(&packet, VISCA_WB_ONE_PUSH_TRIG);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setRgainUp() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_RGAIN);
        appendByte(&packet, VISCA_UP);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setRgainDown() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_RGAIN);
        appendByte(&packet, VISCA_DOWN);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setRgainReset() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_RGAIN);
        appendByte(&packet, VISCA_RESET);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setRgainValue(const uint8_t value) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_RGAIN_VALUE);
        appendAsNibbles(&packet, value);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setBgainUp() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BGAIN);
        appendByte(&packet, VISCA_UP);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setBgainDown() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BGAIN);
        appendByte(&packet, VISCA_DOWN);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setBgainReset() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BGAIN);
        appendByte(&packet, VISCA_RESET);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setBgainValue(const uint8_t value) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BGAIN_VALUE);
        appendAsNibbles(&packet, value);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setShutterUp() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_SHUTTER);
        appendByte(&packet, VISCA_UP);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setShutterDown() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_SHUTTER);
        appendByte(&packet, VISCA_DOWN);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setShutterReset() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_SHUTTER);
        appendByte(&packet, VISCA_RESET);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setShutterValue(const uint8_t value) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_SHUTTER_VALUE);
        appendAsNibbles(&packet, value);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setIrisUp() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_IRIS);
        appendByte(&packet, VISCA_UP);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setIrisDown() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_IRIS);
        appendByte(&packet, VISCA_DOWN);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setIrisReset() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_IRIS);
        appendByte(&packet, VISCA_RESET);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setIrisValue(const uint8_t value) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_IRIS_VALUE);
        appendAsNibbles(&packet, value);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setGainUp() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_GAIN);
        appendByte(&packet, VISCA_UP);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setGainDown() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_GAIN);
        appendByte(&packet, VISCA_DOWN);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setGainReset() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_GAIN);
        appendByte(&packet, VISCA_RESET);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setGainValue(const uint8_t value) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_GAIN_VALUE);
        appendAsNibbles(&packet, value);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setBrightUp() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BRIGHT);
        appendByte(&packet, VISCA_UP);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setBrightDown() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BRIGHT);
        appendByte(&packet, VISCA_DOWN);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setBrightReset() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BRIGHT);
        appendByte(&packet, VISCA_RESET);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setBrightValue(const uint16_t value) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BRIGHT_VALUE);
        appendAsNibbles(&packet, value);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setApertureUp() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_APERTURE);
        appendByte(&packet, VISCA_UP);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setApertureDown() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_APERTURE);
        appendByte(&packet, VISCA_DOWN);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setApertureReset() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_APERTURE);
        appendByte(&packet, VISCA_RESET);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setApertureValue(const uint16_t value) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_APERTURE_VALUE);
        appendAsNibbles(&packet, value);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setExpCompUp() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_EXP_COMP);
        appendByte(&packet, VISCA_UP);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setExpCompDown() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_EXP_COMP);
        appendByte(&packet, VISCA_DOWN);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setExpCompReset() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_EXP_COMP);
        appendByte(&packet, VISCA_RESET);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setExpCompValue(const uint8_t value) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_EXP_COMP_VALUE);
        appendAsNibbles(&packet, value);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setExpCompPower(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_EXP_COMP_POWER);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setAutoExpMode(const uint8_t mode) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_AUTO_EXP);
        appendByte(&packet, mode);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setSlowShutterAuto(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_SLOW_SHUTTER);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setBacklightComp(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BACKLIGHT_COMP);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setZeroLuxShot(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ZERO_LUX);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setIrLed(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_IR_LED);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setWideMode(const uint8_t mode) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_WIDE_MODE);
        appendByte(&packet, mode);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setMirror(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_MIRROR);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setFreeze(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FREEZE);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setPictureEffect(const uint8_t mode) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_PICTURE_EFFECT);
        appendByte(&packet, mode);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setDigitalEffect(const uint8_t mode) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DIGITAL_EFFECT);
        appendByte(&packet, mode);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setDigitalEffectLevel(const uint8_t level) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DIGITAL_EFFECT_LEVEL);
        appendByte(&packet, level);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setCamStabilizer(const bool power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_CAM_STABILIZER);

        if (power) {
            appendByte(&packet, VISCA_ON);
        }
        else {
            appendByte(&packet, VISCA_OFF);
        }

        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return Result<void>::error(getViscaErrorMessage(err));
        }

        return Result<void>::success();
    }

    ResultCode Visca::memorySet(const uint8_t channel) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_MEMORY);
        appendByte(&packet, VISCA_MEMORY_SET);
        appendByte(&packet, channel);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::memoryRecall(const uint8_t channel) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_MEMORY);
        appendByte(&packet, VISCA_MEMORY_RECALL);
        appendByte(&packet, channel);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::memoryReset(const uint8_t channel) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_MEMORY);
        appendByte(&packet, VISCA_MEMORY_RESET);
        appendByte(&packet, channel);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setDisplay(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DISPLAY);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setDateTime(const uint16_t year, const uint16_t month, const uint16_t day, const uint16_t hour,
                                  const uint16_t minute) const {
        if (month < 1 || month > 12 || day < 1 || day > 31 || hour > 23 || minute > 59) {
            return ResultCode::Failure;
        }

        ViscaPacket packet{};

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

    ResultCode Visca::setDateDisplay(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DATE_DISPLAY);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setTimeDisplay(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_TIME_DISPLAY);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setTitleDisplay(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_TITLE_DISPLAY);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setTitleClear() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_TITLE_DISPLAY);
        appendByte(&packet, VISCA_TITLE_DISPLAY_CLEAR);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setTitleParams(const ViscaTitleData* title) const {
        ViscaPacket packet{};

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

    ResultCode Visca::setTitle(const ViscaTitleData* title) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_TITLE_SET);
        appendByte(&packet, VISCA_TITLE_SET_PART1);

        for (auto i = 0; i < 10; i++) {
            appendByte(&packet, title->title[i]);
        }

        if (sendPacketWithReply(&packet) != ResultCode::Success) {
            return ResultCode::Failure;
        }

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_TITLE_SET);
        appendByte(&packet, VISCA_TITLE_SET_PART2);

        for (auto i = 0; i < 10; i++) {
            appendByte(&packet, title->title[i + 10]);
        }

        if (sendPacketWithReply(&packet) != ResultCode::Success) {
            return ResultCode::Failure;
        }

        return ResultCode::Success;
    }

    ResultCode Visca::setSpotAeOn() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_SPOT_AE);
        appendByte(&packet, VISCA_ON);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setSpotAeOff() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_SPOT_AE);
        appendByte(&packet, VISCA_OFF);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setSpotAePosition(const uint8_t x_position, const uint8_t y_position) const {
        ViscaPacket packet{};

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

    Result<std::string_view> Visca::getCameraInfo() {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_INTERFACE);
        appendByte(&packet, VISCA_DEVICE_INFO);

        if (sendPacketWithReply(&packet) != ResultCode::Success) {
            return Result<std::string_view>::error("Failed to send getCameraInfo command");
        }

        camera.vendor = (transport_->iface.ibuf[2] << 8) + transport_->iface.ibuf[3];
        const auto vendor_str = getCameraVendor(camera.vendor);
        camera.model = (transport_->iface.ibuf[4] << 8) + transport_->iface.ibuf[5];
        const auto model_str = getCameraModel(camera.model);

        if (vendor_str == "Unknown" || model_str == "Unknown") {
            return Result<std::string_view>::error("Unknown camera");
        }

        camera.rom_version = (transport_->iface.ibuf[6] << 8) + transport_->iface.ibuf[7];
        camera.socket_num = transport_->iface.ibuf[8];

        thread_local std::array<char, 256> buffer{};
        const auto [out, size] = std::format_to_n(buffer.begin(), buffer.size() - 1,
                                                  "{} {}, ROM Version: 0x{:04X}, Socket: 0x{:02X}, Address: 0x{:02X}",
                                                  vendor_str, model_str, camera.rom_version, camera.socket_num,
                                                  camera.address);
        *out = '\0';

        return Result<std::string_view>::success(std::string_view{
            buffer.data(), static_cast<std::size_t>(out - buffer.begin())
        });
    }

    ResultCode Visca::getPower(uint8_t* power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_POWER);
        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return err;
        }
        *power = getByte();
        return ResultCode::Success;
    }

    ResultCode Visca::getDzoomValue(uint8_t* value) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DZOOM);
        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return err;
        }
        *value = getByte();
        return ResultCode::Success;
    }

    ResultCode Visca::getDzoomLimit(uint8_t* value) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DZOOM_LIMIT);
        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return err;
        }
        *value = getByte();
        return ResultCode::Success;
    }

    Result<uint16_t> Visca::getZoomValue() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ZOOM_VALUE);

        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return Result<uint16_t>::error(getViscaErrorMessage(err));
        }
        const uint16_t value = (transport_->iface.ibuf[2] << 12) + (transport_->iface.ibuf[3] << 8) + (transport_->iface
            .ibuf[4] << 4) + transport_->iface.ibuf[5];
        return Result<uint16_t>::success(value);
    }

    Result<bool> Visca::getFocusAuto() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_AUTO);

        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return Result<bool>::error(getViscaErrorMessage(err));
        }

        if (transport_->iface.ibuf[2] == VISCA_OFF) {
            return Result<bool>::success(false);
        }

        return Result<bool>::success(true);
    }

    Result<uint16_t> Visca::getFocusValue() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_VALUE);

        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return Result<uint16_t>::error(getViscaErrorMessage(err));
        }

        const uint16_t value = (transport_->iface.ibuf[2] << 12) + (transport_->iface.ibuf[3] << 8) + (transport_->iface
            .ibuf[4] << 4) + transport_->iface.ibuf[5];
        return Result<uint16_t>::success(value);
    }

    ResultCode Visca::getFocusAutoSense(uint8_t* mode) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_AUTO_SENSE);

        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return err;
        }
        *mode = getByte();
        return ResultCode::Success;
    }

    ResultCode Visca::getFocusNearLimit(uint16_t* value) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_NEAR_LIMIT);

        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return err;
        }
        *value = getFromNibbles();
        return ResultCode::Success;
    }

    ResultCode Visca::getWhitebalMode(uint8_t* mode) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_WB);

        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return err;
        }
        *mode = getByte();
        return ResultCode::Success;
    }

    ResultCode Visca::getRgainValue(uint8_t* value) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_RGAIN_VALUE);

        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return err;
        }
        *value = getFromNibbles();
        return ResultCode::Success;
    }

    ResultCode Visca::getBgainValue(uint8_t* value) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BGAIN_VALUE);

        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return err;
        }
        *value = getFromNibbles();
        return ResultCode::Success;
    }

    ResultCode Visca::getAutoExpMode(uint8_t* mode) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_AUTO_EXP);

        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return err;
        }
        *mode = getByte();
        return ResultCode::Success;
    }

    ResultCode Visca::getSlowShutterAuto(uint8_t* mode) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_SLOW_SHUTTER);

        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return err;
        }
        *mode = getByte();
        return ResultCode::Success;
    }

    ResultCode Visca::getShutterValue(uint8_t* value) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_SHUTTER_VALUE);

        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return err;
        }
        *value = getFromNibbles();
        return ResultCode::Success;
    }

    uint16_t Visca::getFromNibbles() const {
        return (transport_->iface.ibuf[2] << 12) + (transport_->iface.ibuf[3] << 8) + (transport_->iface.ibuf[4] << 4) +
            transport_->iface.ibuf[5];
    }

    ResultCode Visca::getIrisValue(uint8_t* value) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_IRIS_VALUE);

        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return err;
        }
        *value = getFromNibbles();
        return ResultCode::Success;
    }

    ResultCode Visca::getGainValue(uint8_t* value) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_GAIN_VALUE);

        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return err;
        }
        *value = getFromNibbles();
        return ResultCode::Success;
    }

    ResultCode Visca::getBrightValue(uint16_t* value) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BRIGHT_VALUE);

        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return err;
        }
        *value = getFromNibbles();
        return ResultCode::Success;
    }

    ResultCode Visca::getExpCompPower(uint8_t* power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_EXP_COMP_POWER);

        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return err;
        }
        *power = getByte();
        return ResultCode::Success;
    }

    ResultCode Visca::getExpCompValue(uint16_t* value) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_EXP_COMP_VALUE);

        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return err;
        }
        *value = getFromNibbles();
        return ResultCode::Success;
    }

    uint8_t Visca::getByte() const {
        return transport_->iface.ibuf[2];
    }

    ResultCode Visca::getBacklightComp(uint8_t* power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BACKLIGHT_COMP);

        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return err;
        }
        *power = getByte();
        return ResultCode::Success;
    }

    ResultCode Visca::getApertureValue(uint16_t* value) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_APERTURE_VALUE);

        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return err;
        }
        *value = getFromNibbles();
        return ResultCode::Success;
    }

    ResultCode Visca::getZeroLuxShot(uint8_t* power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ZERO_LUX);

        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return err;
        }
        *power = getByte();
        return ResultCode::Success;
    }

    ResultCode Visca::getIrLed(uint8_t* power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_IR_LED);

        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return err;
        }
        *power = getByte();
        return ResultCode::Success;
    }

    ResultCode Visca::getWideMode(uint8_t* mode) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_WIDE_MODE);

        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return err;
        }
        *mode = getByte();
        return ResultCode::Success;
    }

    ResultCode Visca::getMirror(uint8_t* power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_MIRROR);

        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return err;
        }
        *power = getByte();
        return ResultCode::Success;
    }

    ResultCode Visca::getFreeze(uint8_t* power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FREEZE);

        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return err;
        }
        *power = getByte();
        return ResultCode::Success;
    }

    ResultCode Visca::getPictureEffect(uint8_t* mode) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_PICTURE_EFFECT);

        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return err;
        }
        *mode = getByte();
        return ResultCode::Success;
    }

    ResultCode Visca::getDigitalEffect(uint8_t* mode) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DIGITAL_EFFECT);

        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return err;
        }
        *mode = getByte();
        return ResultCode::Success;
    }

    ResultCode Visca::getDigitalEffectLevel(uint16_t* value) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DIGITAL_EFFECT_LEVEL);

        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return err;
        }
        *value = getFromNibbles();
        return ResultCode::Success;
    }

    ResultCode Visca::getMemory(uint8_t* channel) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_MEMORY);

        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return err;
        }
        *channel = getByte();
        return ResultCode::Success;
    }

    ResultCode Visca::getDisplay(uint8_t* power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DISPLAY);

        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return err;
        }
        *power = getByte();
        return ResultCode::Success;
    }

    ResultCode Visca::getId(uint16_t* id) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ID);

        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return err;
        }
        *id = (transport_->iface.ibuf[2] << 12) + (transport_->iface.ibuf[3] << 8) + (transport_->iface.ibuf[4] << 4) +
            transport_->iface.ibuf[5];
        return ResultCode::Success;
    }

    ResultCode Visca::setIrreceiveOn() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_IRRECEIVE);
        appendByte(&packet, VISCA_ON);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setIrreceiveOff() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_IRRECEIVE);
        appendByte(&packet, VISCA_OFF);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setIrreceiveOnoff() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_IRRECEIVE);
        appendByte(&packet, VISCA_IRRECEIVE_ONOFF);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setPanTiltUp(const uint8_t pan_speed, const uint8_t tilt_speed) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_DRIVE);
        appendByte(&packet, pan_speed);
        appendByte(&packet, tilt_speed);
        appendByte(&packet, VISCA_PT_DRIVE_HORIZ_STOP);
        appendByte(&packet, VISCA_PT_DRIVE_VERT_UP);
        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setPanTiltDown(const uint8_t pan_speed, const uint8_t tilt_speed) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_DRIVE);
        appendByte(&packet, pan_speed);
        appendByte(&packet, tilt_speed);
        appendByte(&packet, VISCA_PT_DRIVE_HORIZ_STOP);
        appendByte(&packet, VISCA_PT_DRIVE_VERT_DOWN);
        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setPanTiltLeft(const uint8_t pan_speed, const uint8_t tilt_speed) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_DRIVE);
        appendByte(&packet, pan_speed);
        appendByte(&packet, tilt_speed);
        appendByte(&packet, VISCA_PT_DRIVE_HORIZ_LEFT);
        appendByte(&packet, VISCA_PT_DRIVE_VERT_STOP);
        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setPanTiltRight(const uint8_t pan_speed, const uint8_t tilt_speed) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_DRIVE);
        appendByte(&packet, pan_speed);
        appendByte(&packet, tilt_speed);
        appendByte(&packet, VISCA_PT_DRIVE_HORIZ_RIGHT);
        appendByte(&packet, VISCA_PT_DRIVE_VERT_STOP);
        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setPanTiltUpleft(const uint8_t pan_speed, const uint8_t tilt_speed) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_DRIVE);
        appendByte(&packet, pan_speed);
        appendByte(&packet, tilt_speed);
        appendByte(&packet, VISCA_PT_DRIVE_HORIZ_LEFT);
        appendByte(&packet, VISCA_PT_DRIVE_VERT_UP);
        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setPanTiltUpright(const uint8_t pan_speed, const uint8_t tilt_speed) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_DRIVE);
        appendByte(&packet, pan_speed);
        appendByte(&packet, tilt_speed);
        appendByte(&packet, VISCA_PT_DRIVE_HORIZ_RIGHT);
        appendByte(&packet, VISCA_PT_DRIVE_VERT_UP);
        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setPanTiltDownleft(const uint8_t pan_speed, const uint8_t tilt_speed) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_DRIVE);
        appendByte(&packet, pan_speed);
        appendByte(&packet, tilt_speed);
        appendByte(&packet, VISCA_PT_DRIVE_HORIZ_LEFT);
        appendByte(&packet, VISCA_PT_DRIVE_VERT_DOWN);
        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setPanTiltDownright(const uint8_t pan_speed, const uint8_t tilt_speed) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_DRIVE);
        appendByte(&packet, pan_speed);
        appendByte(&packet, tilt_speed);
        appendByte(&packet, VISCA_PT_DRIVE_HORIZ_RIGHT);
        appendByte(&packet, VISCA_PT_DRIVE_VERT_DOWN);
        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setPanTiltStop(const uint8_t pan_speed, const uint8_t tilt_speed) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_DRIVE);
        appendByte(&packet, pan_speed);
        appendByte(&packet, tilt_speed);
        appendByte(&packet, VISCA_PT_DRIVE_HORIZ_STOP);
        appendByte(&packet, VISCA_PT_DRIVE_VERT_STOP);
        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setPanTiltAbsolutePosition(const uint8_t pan_speed, const uint8_t tilt_speed,
                                                 const uint16_t pan_pos, const uint16_t tilt_pos) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_ABSOLUTE_POSITION);
        appendByte(&packet, pan_speed);
        appendByte(&packet, tilt_speed);
        appendAsNibbles(&packet, pan_pos);
        appendAsNibbles(&packet, tilt_pos);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setPanTiltRelativePosition(const uint8_t pan_speed, const uint8_t tilt_speed,
                                                 const uint16_t pan_pos, const uint16_t tilt_pos) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_RELATIVE_POSITION);
        appendByte(&packet, pan_speed);
        appendByte(&packet, tilt_speed);

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

    ResultCode Visca::setPanTiltHome() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_HOME);
        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setPanTiltReset() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_RESET);
        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setPanTiltLimitUpright(const uint16_t pan_limit, const uint16_t tilt_limit) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_LIMITSET);
        appendByte(&packet, VISCA_PT_LIMITSET_SET);
        appendByte(&packet, VISCA_PT_LIMITSET_SET_UR);
        appendAsNibbles(&packet, pan_limit);
        appendAsNibbles(&packet, tilt_limit);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setPanTiltLimitDownleft(const uint16_t pan_limit, const uint16_t tilt_limit) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_LIMITSET);
        appendByte(&packet, VISCA_PT_LIMITSET_SET);
        appendByte(&packet, VISCA_PT_LIMITSET_SET_DL);
        appendAsNibbles(&packet, pan_limit);
        appendAsNibbles(&packet, tilt_limit);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setPanTiltLimitDownleftClear() const {
        ViscaPacket packet{};

        constexpr uint16_t pan_lmit = 0x7fff;
        constexpr uint16_t tilt_limit = 0x7fff;

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_LIMITSET);
        appendByte(&packet, VISCA_PT_LIMITSET_CLEAR);
        appendByte(&packet, VISCA_PT_LIMITSET_SET_DL);
        appendAsNibbles(&packet, pan_lmit);
        appendAsNibbles(&packet, tilt_limit);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setPanTiltLimitUprightClear() const {
        ViscaPacket packet{};

        constexpr uint16_t pan_limit = 0x7fff;
        constexpr uint16_t tilt_limit = 0x7fff;

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_LIMITSET);
        appendByte(&packet, VISCA_PT_LIMITSET_CLEAR);
        appendByte(&packet, VISCA_PT_LIMITSET_SET_UR);
        appendAsNibbles(&packet, pan_limit);
        appendAsNibbles(&packet, tilt_limit);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setDatascreenOn() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_DATASCREEN);
        appendByte(&packet, VISCA_ON);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setDatascreenOff() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_DATASCREEN);
        appendByte(&packet, VISCA_OFF);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setDatascreenOnoff() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_DATASCREEN);
        appendByte(&packet, VISCA_PT_DATASCREEN_ONOFF);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setRegister(const uint8_t reg_num, const uint8_t reg_val) const {
        ViscaPacket packet{};

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

    ResultCode Visca::getVideoSystem(uint8_t* system) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_VIDEOSYSTEM_INQ);

        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return err;
        }
        *system = getByte();
        return ResultCode::Success;
    }

    ResultCode Visca::getPanTiltMode(uint16_t* status) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_MODE_INQ);

        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return err;
        }
        *status = ((transport_->iface.ibuf[2] & 0xff) << 8) + (transport_->iface.ibuf[3] & 0xff);
        return ResultCode::Success;
    }

    ResultCode Visca::getPanTiltMaxspeed(uint8_t* max_pan_speed, uint8_t* max_tilt_speed) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_MAXSPEED_INQ);

        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return err;
        }
        *max_pan_speed = (transport_->iface.ibuf[2] & 0xff);
        *max_tilt_speed = (transport_->iface.ibuf[3] & 0xff);
        return ResultCode::Success;
    }

    ResultCode Visca::getPanTiltPosition(uint16_t* pan_position, uint16_t* tilt_position) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_POSITION_INQ);

        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return err;
        }
        *pan_position = ((transport_->iface.ibuf[2] & 0xf) << 12) + ((transport_->iface.ibuf[3] & 0xf) << 8) + ((
            transport_->iface.ibuf[4] & 0xf) << 4) + (transport_->iface.ibuf[5] & 0xf);
        *tilt_position = ((transport_->iface.ibuf[6] & 0xf) << 12) + ((transport_->iface.ibuf[7] & 0xf) << 8) + ((
            transport_->iface.ibuf[8] & 0xf) << 4) + (transport_->iface.ibuf[9] & 0xf);

        return ResultCode::Success;
    }

    ResultCode Visca::getDatascreen(uint8_t* status) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_DATASCREEN_INQ);

        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return err;
        }
        *status = getByte();
        return ResultCode::Success;
    }

    ResultCode Visca::getRegister(const uint8_t reg_num, uint8_t* reg_val) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_REGISTER_VALUE);
        appendByte(&packet, reg_num);

        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return err;
        }
        *reg_val = (transport_->iface.ibuf[2] << 4) + transport_->iface.ibuf[3];
        return ResultCode::Success;
    }

    /********************************/
    /* SPECIAL FUNCTIONS FOR D30/31 */
    /********************************/

    ResultCode Visca::setWideConLens(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_WIDE_CON_LENS);
        appendByte(&packet, VISCA_WIDE_CON_LENS_SET);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setAtModeOnoff() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_MODE);
        appendByte(&packet, VISCA_AT_ONOFF);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setAtMode(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_MODE);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setAtAeOnoff() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_AE);
        appendByte(&packet, VISCA_AT_ONOFF);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setAtAe(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_AE);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setAtAutozoomOnoff() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_AUTOZOOM);
        appendByte(&packet, VISCA_AT_ONOFF);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setAtAutozoom(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_AUTOZOOM);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setAtmdFramedisplayOnoff() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_ATMD_FRAMEDISPLAY);
        appendByte(&packet, VISCA_AT_ONOFF);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setAtmdFramedisplay(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_ATMD_FRAMEDISPLAY);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setAtFrameoffsetOnoff() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_FRAMEOFFSET);
        appendByte(&packet, VISCA_AT_ONOFF);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setAtFrameoffset(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_FRAMEOFFSET);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setAtmdStartstop() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_ATMD_STARTSTOP);
        appendByte(&packet, VISCA_AT_ONOFF);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setAtChase(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_CHASE);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setAtChaseNext() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_CHASE);
        appendByte(&packet, VISCA_AT_CHASE_NEXT);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setMdModeOnoff() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_MODE);
        appendByte(&packet, VISCA_MD_ONOFF);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setMdMode(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_MODE);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setMdFrame() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_FRAME);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setMdDetect() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_DETECT);
        appendByte(&packet, VISCA_MD_ONOFF);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setAtEntry(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_ENTRY);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setAtLostinfo() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_ATMD_LOSTINFO1);
        appendByte(&packet, VISCA_ATMD_LOSTINFO2);
        appendByte(&packet, VISCA_AT_LOSTINFO);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setMdLostinfo() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_ATMD_LOSTINFO1);
        appendByte(&packet, VISCA_ATMD_LOSTINFO2);
        appendByte(&packet, VISCA_MD_LOSTINFO);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setMdAdjustYlevel(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_YLEVEL);
        appendByte(&packet, VISCA_MD_ADJUST);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setMdAdjustHuelevel(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_HUELEVEL);
        appendByte(&packet, VISCA_MD_ADJUST);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setMdAdjustSize(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_SIZE);
        appendByte(&packet, VISCA_MD_ADJUST);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setMdAdjustDisptime(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_DISPTIME);
        appendByte(&packet, VISCA_MD_ADJUST);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setMdAdjustRefmode(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_REFMODE);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setMdAdjustReftime(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_REFTIME);
        appendByte(&packet, VISCA_MD_ADJUST);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setMdMeasureMode1Onoff() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_MEASURE_MODE_1);
        appendByte(&packet, VISCA_MD_ONOFF);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setMdMeasureMode1(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_MEASURE_MODE_1);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setMdMeasureMode2Onoff() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_MEASURE_MODE_2);
        appendByte(&packet, VISCA_MD_ONOFF);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::setMdMeasureMode2(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_MEASURE_MODE_2);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    ResultCode Visca::getKeylock(uint8_t* power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_KEYLOCK);

        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return err;
        }
        *power = getByte();
        return ResultCode::Success;
    }

    ResultCode Visca::getWideConLens(uint8_t* power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_WIDE_CON_LENS);

        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return err;
        }
        *power = getByte();
        return ResultCode::Success;
    }

    ResultCode Visca::getAtmdMode(uint8_t* power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_ATMD_MODE);

        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return err;
        }
        *power = getByte();
        return ResultCode::Success;
    }

    ResultCode Visca::getAtMode(uint16_t* value) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_MODE_QUERY);

        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return err;
        }
        *value = ((transport_->iface.ibuf[2] & 0xff) << 8) + (transport_->iface.ibuf[3] & 0xff);
        return ResultCode::Success;
    }

    ResultCode Visca::getAtEntry(uint8_t* power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_ENTRY);

        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return err;
        }
        *power = getByte();
        return ResultCode::Success;
    }

    ResultCode Visca::getMdMode(uint16_t* value) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_MODE_QUERY);
        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return err;
        }
        *value = ((transport_->iface.ibuf[2] & 0xff) << 8) + (transport_->iface.ibuf[3] & 0xff);
        return ResultCode::Success;
    }

    ResultCode Visca::getMdYlevel(uint8_t* power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_YLEVEL);
        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return err;
        }
        *power = (transport_->iface.ibuf[3] & 0x0f);
        return ResultCode::Success;
    }

    ResultCode Visca::getMdHuelevel(uint8_t* power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_HUELEVEL);
        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return err;
        }
        *power = (transport_->iface.ibuf[3] & 0x0f);
        return ResultCode::Success;
    }

    ResultCode Visca::getMdSize(uint8_t* power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_SIZE);
        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return err;
        }
        *power = (transport_->iface.ibuf[3] & 0x0f);
        return ResultCode::Success;
    }

    ResultCode Visca::getMdDisptime(uint8_t* power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_DISPTIME);

        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return err;
        }
        *power = (transport_->iface.ibuf[3] & 0x0f);
        return ResultCode::Success;
    }

    ResultCode Visca::getMdRefmode(uint8_t* power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_REFMODE);

        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return err;
        }
        *power = getByte();
        return ResultCode::Success;
    }

    ResultCode Visca::getMdReftime(uint8_t* power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_REFTIME_QUERY);

        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return err;
        }
        *power = (transport_->iface.ibuf[3] & 0x0f);
        return ResultCode::Success;
    }

    ResultCode Visca::getAtObjPos(uint8_t* xpos, uint8_t* ypos, uint8_t* status) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_POSITION);

        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return err;
        }
        *xpos = getByte();
        *ypos = transport_->iface.ibuf[3];
        *status = (transport_->iface.ibuf[4] & 0x0f);
        return ResultCode::Success;
    }

    ResultCode Visca::getMdObjPos(uint8_t* xpos, uint8_t* ypos, uint8_t* status) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_POSITION);

        if (const auto err = sendPacketWithReply(&packet); err != ResultCode::Success) {
            return err;
        }
        *xpos = getByte();
        *ypos = transport_->iface.ibuf[3];
        *status = (transport_->iface.ibuf[4] & 0x0f);
        return ResultCode::Success;
    }

    std::string Visca::getViscaErrorMessage(const ResultCode error_code) {
        switch (error_code) {
            case ResultCode::ErrorMessageLength:
                return "Invalid message length";
            case ResultCode::ErrorSyntax:
                return "Syntax error";
            case ResultCode::ErrorCmdBufferFull:
                return "Command buffer full";
            case ResultCode::ErrorCmdCancelled:
                return "Command cancelled";
            case ResultCode::ErrorNoSocket:
                return "No socket available";
            case ResultCode::ErrorCmdNotExecutable:
                return "Command not executable";
            default:
                return "Unknown error: " + std::to_string(static_cast<uint32_t>(error_code));
        }
    }
}
