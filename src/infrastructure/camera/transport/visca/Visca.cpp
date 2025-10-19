#include "Visca.h"

#include <algorithm>
#include <unistd.h>
#include <sys/ioctl.h>

namespace camera_service::infrastructure {
    Visca::Visca(std::unique_ptr<Uart> transport)
        : transport_(std::move(transport)) {}

    Result<void> Visca::write(const ViscaPacket* packet) const {
        return transport_->write(
            std::span<const std::byte>(reinterpret_cast<const std::byte*>(packet->data.data()), packet->size));
    }

    void Visca::appendHeader(ViscaPacket* packet) const {
        packet->data.at(0) = VISCA_START_BYTE;
        packet->data.at(0) |= (transport_->iface.address << 4);
        if (transport_->iface.broadcast > 0) {
            packet->data.at(0) |= (transport_->iface.broadcast << 3);
            packet->data.at(0) &= 0xF8;
        } else {
            packet->data.at(0) |= cam_address_;
        }
    }

    void Visca::appendTerminator(ViscaPacket* packet) const {
        packet->data.at(packet->size++) = VISCA_TERMINATOR;
    }

    Result<void> Visca::sendPacket(ViscaPacket* packet) const {
        appendHeader(packet);
        appendTerminator(packet);

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
            transport_->iface.ibuf.at(i) = static_cast<uint8_t>(data.at(i));
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
        packet->data.at(packet->size++) = byte;
    }

    void Visca::appendAsNibbles(ViscaPacket* packet, const uint16_t value) {
        appendByte(packet, (value & 0xF000) >> 12);
        appendByte(packet, (value & 0x0F00) >> 8);
        appendByte(packet, (value & 0x00F0) >> 4);
        appendByte(packet, (value & 0x000F));
    }

    Result<ResponseType> Visca::getReply() const {
        if (read() != ResultCode::Success) {
            return Result<ResponseType>::error("Failed to read first reply from camera");
        }
        auto type = static_cast<ResponseType>(transport_->iface.ibuf.at(1) & 0xF0);

        while (type == ResponseType::Ack) {
            if (read() != ResultCode::Success) {
                return Result<ResponseType>::error("Failed to read second reply from camera");
            }
            type = static_cast<ResponseType>(transport_->iface.ibuf.at(1) & 0xF0);
        }

        switch (type) {
            case ResponseType::Clear:
            case ResponseType::Address:
            case ResponseType::Completed:
            case ResponseType::Error:
                return Result<ResponseType>::success(type);
                break;
            default:
                return Result<ResponseType>::error("Unknown response type from camera");
        }
    }

    Result<void> Visca::sendPacketWithReply(ViscaPacket* packet) const {
        if (sendPacket(packet).isError()) {
            return Result<void>::error("Failed to send packet");
        }

        if (const auto result = getReply(); result.isError()) {
            return Result<void>::error(result.error());
        } else if (result.value() == ResponseType::Error) {
            return Result<void>::error(getViscaErrorMessage(static_cast<ResultCode>(transport_->iface.ibuf.at(2))));
        }

        return Result<void>::success();
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
        uint8_t camera_num = 0;

        const auto backup = transport_->iface.broadcast;

        appendByte(&packet, 0x30);
        appendByte(&packet, 0x01);

        transport_->iface.broadcast = 1;
        if (sendPacket(&packet).isError()) {
            transport_->iface.broadcast = backup;
            return Result<void>::error("Failed to send setAddress command");
        }
        transport_->iface.broadcast = backup;

        if (getReply().isError()) {
            return Result<void>::error("Failed to get reply for setAddress command");
        }
        /* We parse the message from the camera here  */
        /* We expect to receive 4*camera_num bytes,
               every packet should be 88 30 0x FF, x being
               the camera id+1. The number of cams will thus be
               ibuf.at(bytes-2)-1  */
        if ((transport_->iface.size & 0x3) != 0) {
            /* check multiple of 4 */
            return Result<void>::error("Invalid response length for setAddress command");
        }
        camera_num = transport_->iface.ibuf.at(transport_->iface.size - 2) - 1;
        if ((camera_num == 0) || (camera_num > 7)) {
            return Result<void>::error("Invalid number of cameras detected");
        }
        cam_address_ = camera_num;
        return Result<void>::success();
    }

    Result<void> Visca::clear() const {
        ViscaPacket packet{};

        appendByte(&packet, 0x01);
        appendByte(&packet, 0x00);
        appendByte(&packet, 0x01);

        if (sendPacket(&packet).isError()) {
            return Result<void>::error("Failed to send clear command");
        }
        if (getReply().isError()) {
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
            case CameraModels::EW9500H:
                return "EW9500H";
            default:
                return "Unknown";
        }
    }

    Result<void> Visca::setPower(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_POWER);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setKeylock(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_KEYLOCK);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setCameraId(const uint16_t id) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ID);
        appendAsNibbles(&packet, id);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setZoomTele() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ZOOM);
        appendByte(&packet, VISCA_ZOOM_TELE);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setZoomWide() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ZOOM);
        appendByte(&packet, VISCA_ZOOM_WIDE);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setZoomStop() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ZOOM);
        appendByte(&packet, VISCA_ZOOM_STOP);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setZoomTeleSpeed(const uint32_t speed) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ZOOM);
        appendByte(&packet, VISCA_ZOOM_TELE_SPEED | (speed & 0x7));

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setZoomWideSpeed(const uint32_t speed) const {
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

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setZoomAndFocusValue(const uint16_t zoom, const uint16_t focus) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ZOOM_FOCUS_VALUE);
        appendAsNibbles(&packet, zoom);
        appendAsNibbles(&packet, focus);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setDzoomValue(const uint8_t value) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DZOOM_VALUE);
        appendAsNibbles(&packet, value);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setDzoomLimit(const uint8_t limit) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DZOOM_LIMIT);
        appendByte(&packet, limit);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setDzoomMode(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DZOOM_MODE);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setFocusFar() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS);
        appendByte(&packet, VISCA_FOCUS_FAR);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setFocusNear() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS);
        appendByte(&packet, VISCA_FOCUS_NEAR);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setFocusStop() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS);
        appendByte(&packet, VISCA_FOCUS_STOP);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setFocusFarSpeed(const uint32_t speed) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS);
        appendByte(&packet, VISCA_FOCUS_FAR_SPEED | (speed & 0x7));

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setFocusNearSpeed(const uint32_t speed) const {
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

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setFocusAuto(const bool on) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_AUTO);
        appendByte(&packet, on ? VISCA_ON : VISCA_OFF);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setFocusOnePush() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_ONE_PUSH);
        appendByte(&packet, VISCA_FOCUS_ONE_PUSH_TRIG);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setFocusInfinity() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_ONE_PUSH);
        appendByte(&packet, VISCA_FOCUS_ONE_PUSH_INF);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setFocusAutosenseHigh() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_AUTO_SENSE);
        appendByte(&packet, VISCA_FOCUS_AUTO_SENSE_HIGH);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setFocusAutosenseLow() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_AUTO_SENSE);
        appendByte(&packet, VISCA_FOCUS_AUTO_SENSE_LOW);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setFocusNearLimit(const uint16_t limit) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_NEAR_LIMIT);
        appendAsNibbles(&packet, limit);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setWhitebalMode(const uint8_t mode) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_WB);
        appendByte(&packet, mode);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setWhitebalOnePush() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_WB_TRIGGER);
        appendByte(&packet, VISCA_WB_ONE_PUSH_TRIG);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setRgainUp() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_RGAIN);
        appendByte(&packet, VISCA_UP);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setRgainDown() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_RGAIN);
        appendByte(&packet, VISCA_DOWN);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setRgainReset() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_RGAIN);
        appendByte(&packet, VISCA_RESET);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setRgainValue(const uint8_t value) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_RGAIN_VALUE);
        appendAsNibbles(&packet, value);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setBgainUp() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BGAIN);
        appendByte(&packet, VISCA_UP);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setBgainDown() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BGAIN);
        appendByte(&packet, VISCA_DOWN);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setBgainReset() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BGAIN);
        appendByte(&packet, VISCA_RESET);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setBgainValue(const uint8_t value) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BGAIN_VALUE);
        appendAsNibbles(&packet, value);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setShutterUp() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_SHUTTER);
        appendByte(&packet, VISCA_UP);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setShutterDown() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_SHUTTER);
        appendByte(&packet, VISCA_DOWN);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setShutterReset() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_SHUTTER);
        appendByte(&packet, VISCA_RESET);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setShutterValue(const uint8_t value) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_SHUTTER_VALUE);
        appendAsNibbles(&packet, value);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setIrisUp() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_IRIS);
        appendByte(&packet, VISCA_UP);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setIrisDown() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_IRIS);
        appendByte(&packet, VISCA_DOWN);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setIrisReset() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_IRIS);
        appendByte(&packet, VISCA_RESET);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setIrisValue(const uint8_t value) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_IRIS_VALUE);
        appendAsNibbles(&packet, value);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setGainUp() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_GAIN);
        appendByte(&packet, VISCA_UP);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setGainDown() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_GAIN);
        appendByte(&packet, VISCA_DOWN);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setGainReset() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_GAIN);
        appendByte(&packet, VISCA_RESET);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setGainValue(const uint8_t value) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_GAIN_VALUE);
        appendAsNibbles(&packet, value);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setBrightUp() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BRIGHT);
        appendByte(&packet, VISCA_UP);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setBrightDown() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BRIGHT);
        appendByte(&packet, VISCA_DOWN);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setBrightReset() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BRIGHT);
        appendByte(&packet, VISCA_RESET);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setBrightValue(const uint16_t value) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BRIGHT_VALUE);
        appendAsNibbles(&packet, value);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setApertureUp() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_APERTURE);
        appendByte(&packet, VISCA_UP);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setApertureDown() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_APERTURE);
        appendByte(&packet, VISCA_DOWN);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setApertureReset() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_APERTURE);
        appendByte(&packet, VISCA_RESET);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setApertureValue(const uint8_t value) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_APERTURE_VALUE);
        appendAsNibbles(&packet, value);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setExpCompUp() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_EXP_COMP);
        appendByte(&packet, VISCA_UP);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setExpCompDown() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_EXP_COMP);
        appendByte(&packet, VISCA_DOWN);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setExpCompReset() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_EXP_COMP);
        appendByte(&packet, VISCA_RESET);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setExpCompValue(const uint8_t value) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_EXP_COMP_VALUE);
        appendAsNibbles(&packet, value);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setExpCompPower(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_EXP_COMP_POWER);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setAutoExpMode(const uint8_t mode) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_AUTO_EXP);
        appendByte(&packet, mode);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setSlowShutterAuto(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_SLOW_SHUTTER);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setBacklightComp(const bool on) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BACKLIGHT_COMP);
        if (on) {
            appendByte(&packet, VISCA_ON);
        } else {
            appendByte(&packet, VISCA_OFF);
        }

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setZeroLuxShot(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ZERO_LUX);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setIrLed(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_IR_LED);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setWideMode(const uint8_t mode) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_WIDE_MODE);
        appendByte(&packet, mode);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setMirror(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_MIRROR);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setFreeze(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FREEZE);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setPictureEffect(const uint8_t mode) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_PICTURE_EFFECT);
        appendByte(&packet, mode);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setDigitalEffect(const uint8_t mode) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DIGITAL_EFFECT);
        appendByte(&packet, mode);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setDigitalEffectLevel(const uint8_t level) const {
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
        } else {
            appendByte(&packet, VISCA_OFF);
        }

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::memorySet(const uint8_t channel) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_MEMORY);
        appendByte(&packet, VISCA_MEMORY_SET);
        appendByte(&packet, channel);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::memoryRecall(const uint8_t channel) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_MEMORY);
        appendByte(&packet, VISCA_MEMORY_RECALL);
        appendByte(&packet, channel);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::memoryReset(const uint8_t channel) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_MEMORY);
        appendByte(&packet, VISCA_MEMORY_RESET);
        appendByte(&packet, channel);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setDisplay(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DISPLAY);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setDateTime(const uint16_t year, const uint16_t month, const uint16_t day, const uint16_t hour,
                                    const uint16_t minute) const {
        if (month < 1 || month > 12 || day < 1 || day > 31 || hour > 23 || minute > 59) {
            return Result<void>::error("Invalid input");
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

    Result<void> Visca::setDateDisplay(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DATE_DISPLAY);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setTimeDisplay(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_TIME_DISPLAY);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setTitleDisplay(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_TITLE_DISPLAY);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setTitleClear() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_TITLE_DISPLAY);
        appendByte(&packet, VISCA_TITLE_DISPLAY_CLEAR);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setTitleParams(const ViscaTitleData* title) const {
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

    Result<void> Visca::setTitle(const ViscaTitleData* title) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_TITLE_SET);
        appendByte(&packet, VISCA_TITLE_SET_PART1);

        for (auto i = 0; i < 10; i++) {
            appendByte(&packet, title->title.at(i));
        }

        if (const auto result = sendPacketWithReply(&packet); result.isError()) {
            return Result<void>::error(result.error());
        }

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_TITLE_SET);
        appendByte(&packet, VISCA_TITLE_SET_PART2);

        for (auto i = 0; i < 10; i++) {
            appendByte(&packet, title->title.at(i + 10));
        }

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setSpotAeOn() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_SPOT_AE);
        appendByte(&packet, VISCA_ON);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setSpotAeOff() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_SPOT_AE);
        appendByte(&packet, VISCA_OFF);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setSpotAePosition(const uint8_t x_position, const uint8_t y_position) const {
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

        if (const auto result = sendPacketWithReply(&packet); result.isError()) {
            return Result<std::string_view>::error(result.error());
        }

        const uint16_t vendor = (transport_->iface.ibuf.at(2) << 8) + transport_->iface.ibuf.at(3);
        const auto vendor_str = getCameraVendor(vendor);
        const uint16_t model = (transport_->iface.ibuf.at(4) << 8) + transport_->iface.ibuf.at(5);
        const auto model_str = getCameraModel(model);

        if (vendor_str == "Unknown" || model_str == "Unknown") {
            return Result<std::string_view>::error("Unknown camera");
        }

        const uint16_t rom_version = (transport_->iface.ibuf.at(6) << 8) + transport_->iface.ibuf.at(7);
        const uint8_t socket_num = transport_->iface.ibuf.at(8);

        thread_local std::array<char, 256> buffer{};
        const auto [out, size] = std::format_to_n(buffer.begin(), buffer.size() - 1,
                                                  "{} {}, ROM Version: 0x{:04X}, Socket: 0x{:02X}, Address: 0x{:02X}",
                                                  vendor_str, model_str, rom_version, socket_num, cam_address_);
        *out = '\0';

        return Result<std::string_view>::success(std::string_view{
            buffer.data(), static_cast<std::size_t>(out - buffer.begin())
        });
    }

    Result<void> Visca::setIrreceiveOn() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_IRRECEIVE);
        appendByte(&packet, VISCA_ON);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setIrreceiveOff() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_IRRECEIVE);
        appendByte(&packet, VISCA_OFF);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setIrreceiveOnoff() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_IRRECEIVE);
        appendByte(&packet, VISCA_IRRECEIVE_ONOFF);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setPanTiltUp(const uint8_t pan_speed, const uint8_t tilt_speed) const {
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

    Result<void> Visca::setPanTiltDown(const uint8_t pan_speed, const uint8_t tilt_speed) const {
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

    Result<void> Visca::setPanTiltLeft(const uint8_t pan_speed, const uint8_t tilt_speed) const {
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

    Result<void> Visca::setPanTiltRight(const uint8_t pan_speed, const uint8_t tilt_speed) const {
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

    Result<void> Visca::setPanTiltUpleft(const uint8_t pan_speed, const uint8_t tilt_speed) const {
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

    Result<void> Visca::setPanTiltUpright(const uint8_t pan_speed, const uint8_t tilt_speed) const {
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

    Result<void> Visca::setPanTiltDownleft(const uint8_t pan_speed, const uint8_t tilt_speed) const {
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

    Result<void> Visca::setPanTiltDownright(const uint8_t pan_speed, const uint8_t tilt_speed) const {
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

    Result<void> Visca::setPanTiltStop(const uint8_t pan_speed, const uint8_t tilt_speed) const {
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

    Result<void> Visca::setPanTiltAbsolutePosition(const uint8_t pan_speed, const uint8_t tilt_speed,
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

    Result<void> Visca::setPanTiltRelativePosition(const uint8_t pan_speed, const uint8_t tilt_speed,
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

    Result<void> Visca::setPanTiltHome() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_HOME);
        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setPanTiltReset() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_RESET);
        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setPanTiltLimitUpright(const uint16_t pan_limit, const uint16_t tilt_limit) const {
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

    Result<void> Visca::setPanTiltLimitDownleft(const uint16_t pan_limit, const uint16_t tilt_limit) const {
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

    Result<void> Visca::setPanTiltLimitDownleftClear() const {
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

    Result<void> Visca::setPanTiltLimitUprightClear() const {
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

    Result<void> Visca::setDatascreenOn() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_DATASCREEN);
        appendByte(&packet, VISCA_ON);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setDatascreenOff() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_DATASCREEN);
        appendByte(&packet, VISCA_OFF);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setDatascreenOnoff() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_DATASCREEN);
        appendByte(&packet, VISCA_PT_DATASCREEN_ONOFF);

        return sendPacketWithReply(&packet);
    }

    Result<uint8_t> Visca::getPower() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_POWER);
        if (const auto result = sendPacketWithReply(&packet); result.isError()) {
            return Result<uint8_t>::error(result.error());
        }
        return Result<uint8_t>::success(getByte());
    }

    Result<uint8_t> Visca::getDzoomValue() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DZOOM);
        if (const auto result = sendPacketWithReply(&packet); result.isError()) {
            return Result<uint8_t>::error(result.error());
        }
        return Result<uint8_t>::success(getByte());
    }

    Result<uint8_t> Visca::getDzoomLimit() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DZOOM_LIMIT);
        if (const auto result = sendPacketWithReply(&packet); result.isError()) {
            return Result<uint8_t>::error(result.error());
        }
        return Result<uint8_t>::success(getByte());
    }

    Result<uint16_t> Visca::getZoomValue() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ZOOM_VALUE);

        if (const auto result = sendPacketWithReply(&packet); result.isError()) {
            return Result<uint16_t>::error(result.error());
        }
        return Result<uint16_t>::success(getFromNibbles());
    }

    Result<bool> Visca::getFocusAuto() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_AUTO);

        if (const auto result = sendPacketWithReply(&packet); result.isError()) {
            return Result<bool>::error(result.error());
        }

        if (transport_->iface.ibuf.at(2) == VISCA_OFF) {
            return Result<bool>::success(false);
        }
        return Result<bool>::success(true);
    }

    Result<uint16_t> Visca::getFocusValue() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_VALUE);

        if (const auto result = sendPacketWithReply(&packet); result.isError()) {
            return Result<uint16_t>::error(result.error());
        }
        return Result<uint16_t>::success(getFromNibbles());
    }

    Result<uint8_t> Visca::getFocusAutoSense() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_AUTO_SENSE);

        if (const auto result = sendPacketWithReply(&packet); result.isError()) {
            return Result<uint8_t>::error(result.error());
        }
        return Result<uint8_t>::success(getByte());
    }

    Result<uint16_t> Visca::getFocusNearLimit() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_NEAR_LIMIT);

        if (const auto result = sendPacketWithReply(&packet); result.isError()) {
            return Result<uint16_t>::error(result.error());
        }
        return Result<uint16_t>::success(getFromNibbles());
    }

    Result<uint8_t> Visca::getWhitebalMode() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_WB);

        if (const auto result = sendPacketWithReply(&packet); result.isError()) {
            return Result<uint8_t>::error(result.error());
        }
        return Result<uint8_t>::success(getByte());
    }

    Result<uint8_t> Visca::getRgainValue() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_RGAIN_VALUE);

        if (const auto result = sendPacketWithReply(&packet); result.isError()) {
            return Result<uint8_t>::error(result.error());
        }
        return Result<uint8_t>::success(getByte());
    }

    Result<uint8_t> Visca::getBgainValue() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BGAIN_VALUE);

        if (const auto result = sendPacketWithReply(&packet); result.isError()) {
            return Result<uint8_t>::error(result.error());
        }
        return Result<uint8_t>::success(getByte());
    }

    Result<uint8_t> Visca::getAutoExpMode() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_AUTO_EXP);

        if (const auto result = sendPacketWithReply(&packet); result.isError()) {
            return Result<uint8_t>::error(result.error());
        }
        return Result<uint8_t>::success(getByte());
    }

    Result<uint8_t> Visca::getSlowShutterAuto() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_SLOW_SHUTTER);

        if (const auto result = sendPacketWithReply(&packet); result.isError()) {
            return Result<uint8_t>::error(result.error());
        }
        return Result<uint8_t>::success(getByte());
    }

    Result<uint8_t> Visca::getShutterValue() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_SHUTTER_VALUE);

        if (const auto result = sendPacketWithReply(&packet); result.isError()) {
            return Result<uint8_t>::error(result.error());
        }
        return Result<uint8_t>::success(static_cast<uint8_t>(getFromNibbles()));
    }

    Result<uint8_t> Visca::getIrisValue() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_IRIS_VALUE);

        if (const auto result = sendPacketWithReply(&packet); result.isError()) {
            return Result<uint8_t>::error(result.error());
        }
        return Result<uint8_t>::success(static_cast<uint8_t>(getFromNibbles()));
    }

    Result<uint8_t> Visca::getGainValue() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_GAIN_VALUE);

        if (const auto result = sendPacketWithReply(&packet); result.isError()) {
            return Result<uint8_t>::error(result.error());
        }
        return Result<uint8_t>::success(static_cast<uint8_t>(getFromNibbles()));
    }

    Result<uint16_t> Visca::getBrightValue() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BRIGHT_VALUE);

        if (const auto result = sendPacketWithReply(&packet); result.isError()) {
            return Result<uint16_t>::error(result.error());
        }
        return Result<uint16_t>::success(getFromNibbles());
    }

    Result<uint8_t> Visca::getExpCompPower() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_EXP_COMP_POWER);

        if (const auto result = sendPacketWithReply(&packet); result.isError()) {
            return Result<uint8_t>::error(result.error());
        }
        return Result<uint8_t>::success(getByte());
    }

    Result<uint8_t> Visca::getExpCompValue() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_EXP_COMP_VALUE);

        if (const auto result = sendPacketWithReply(&packet); result.isError()) {
            return Result<uint8_t>::error(result.error());
        }
        return Result<uint8_t>::success(static_cast<uint8_t>(getFromNibbles()));
    }

    Result<bool> Visca::getBacklightComp() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BACKLIGHT_COMP);

        if (const auto result = sendPacketWithReply(&packet); result.isError()) {
            return Result<bool>::error(result.error());
        }
        if (getByte() == VISCA_OFF) {
            return Result<bool>::success(false);
        }
        return Result<bool>::success(true);
    }

    Result<uint8_t> Visca::getApertureValue() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_APERTURE_VALUE);

        if (const auto result = sendPacketWithReply(&packet); result.isError()) {
            return Result<uint8_t>::error(result.error());
        }
        return Result<uint8_t>::success(static_cast<uint8_t>(getFromNibbles()));
    }

    Result<uint8_t> Visca::getZeroLuxShot() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ZERO_LUX);

        if (const auto result = sendPacketWithReply(&packet); result.isError()) {
            return Result<uint8_t>::error(result.error());
        }
        return Result<uint8_t>::success(getByte());
    }

    Result<uint8_t> Visca::getIrLed() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_IR_LED);

        if (const auto result = sendPacketWithReply(&packet); result.isError()) {
            return Result<uint8_t>::error(result.error());
        }
        return Result<uint8_t>::success(getByte());
    }

    Result<uint8_t> Visca::getWideMode() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_WIDE_MODE);

        if (const auto result = sendPacketWithReply(&packet); result.isError()) {
            return Result<uint8_t>::error(result.error());
        }
        return Result<uint8_t>::success(getByte());
    }

    Result<uint8_t> Visca::getMirror() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_MIRROR);

        if (const auto result = sendPacketWithReply(&packet); result.isError()) {
            return Result<uint8_t>::error(result.error());
        }
        return Result<uint8_t>::success(getByte());
    }

    Result<uint8_t> Visca::getFreeze() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FREEZE);

        if (const auto result = sendPacketWithReply(&packet); result.isError()) {
            return Result<uint8_t>::error(result.error());
        }
        return Result<uint8_t>::success(getByte());
    }

    Result<uint8_t> Visca::getPictureEffect() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_PICTURE_EFFECT);

        if (const auto result = sendPacketWithReply(&packet); result.isError()) {
            return Result<uint8_t>::error(result.error());
        }
        return Result<uint8_t>::success(getByte());
    }

    Result<uint8_t> Visca::getDigitalEffect() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DIGITAL_EFFECT);

        if (const auto result = sendPacketWithReply(&packet); result.isError()) {
            return Result<uint8_t>::error(result.error());
        }
        return Result<uint8_t>::success(getByte());
    }

    Result<uint16_t> Visca::getDigitalEffectLevel() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DIGITAL_EFFECT_LEVEL);

        if (const auto result = sendPacketWithReply(&packet); result.isError()) {
            return Result<uint16_t>::error(result.error());
        }
        return Result<uint16_t>::success(getFromNibbles());
    }

    Result<uint8_t> Visca::getMemory() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_MEMORY);

        if (const auto result = sendPacketWithReply(&packet); result.isError()) {
            return Result<uint8_t>::error(result.error());
        }
        return Result<uint8_t>::success(getByte());
    }

    Result<uint8_t> Visca::getDisplay() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DISPLAY);

        if (const auto result = sendPacketWithReply(&packet); result.isError()) {
            return Result<uint8_t>::error(result.error());
        }
        return Result<uint8_t>::success(getByte());
    }

    Result<uint16_t> Visca::getId() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ID);

        if (const auto result = sendPacketWithReply(&packet); result.isError()) {
            return Result<uint16_t>::error(result.error());
        }
        return Result<uint16_t>::success(getFromNibbles());
    }

    Result<void> Visca::setRegister(const uint8_t reg_num, const uint8_t reg_val) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_REGISTER_VALUE);
        appendByte(&packet, reg_num);
        appendByte(&packet, (reg_val & 0xF0) >> 4);
        appendByte(&packet, (reg_val & 0x0F));
        return sendPacketWithReply(&packet);
    }

    uint16_t Visca::getFromNibbles() const {
        return (transport_->iface.ibuf.at(2) << 12) + (transport_->iface.ibuf.at(3) << 8) + (transport_->iface.ibuf.
            at(4) << 4) + transport_->iface.ibuf.at(5);
    }

    uint8_t Visca::getByte() const {
        return transport_->iface.ibuf.at(2);
    }

    /***********************************/
    /*       INQUIRY FUNCTIONS         */
    /***********************************/

    Result<uint8_t> Visca::getVideoSystem() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_VIDEOSYSTEM_INQ);

        if (const auto result = sendPacketWithReply(&packet); result.isError()) {
            return Result<uint8_t>::error(result.error());
        }
        return Result<uint8_t>::success(getByte());
    }

    Result<uint16_t> Visca::getPanTiltMode() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_MODE_INQ);

        if (const auto result = sendPacketWithReply(&packet); result.isError()) {
            return Result<uint16_t>::error(result.error());
        }
        const uint16_t status = ((transport_->iface.ibuf.at(2) & 0xff) << 8) + (transport_->iface.ibuf.at(3) & 0xff);
        return Result<uint16_t>::success(status);
    }

    Result<std::pair<uint8_t, uint8_t>> Visca::getPanTiltMaxspeed() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_MAXSPEED_INQ);

        if (const auto result = sendPacketWithReply(&packet); result.isError()) {
            return Result<std::pair<uint8_t, uint8_t>>::error(result.error());
        }
        const uint8_t max_pan_speed = (transport_->iface.ibuf.at(2) & 0xff);
        const uint8_t max_tilt_speed = (transport_->iface.ibuf.at(3) & 0xff);
        return Result<std::pair<uint8_t, uint8_t>>::success({max_pan_speed, max_tilt_speed});
    }

    Result<std::pair<uint16_t, uint16_t>> Visca::getPanTiltPosition() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_POSITION_INQ);

        if (const auto result = sendPacketWithReply(&packet); result.isError()) {
            return Result<std::pair<uint16_t, uint16_t>>::error(result.error());
        }
        const uint16_t pan_position = getFromNibbles();
        const uint16_t tilt_position = ((transport_->iface.ibuf.at(6) & 0xf) << 12) + ((transport_->iface.ibuf.at(7) &
            0xf) << 8) + ((transport_->iface.ibuf.at(8) & 0xf) << 4) + (transport_->iface.ibuf.at(9) & 0xf);

        return Result<std::pair<uint16_t, uint16_t>>::success({pan_position, tilt_position});
    }

    Result<uint8_t> Visca::getDatascreen() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_DATASCREEN_INQ);

        if (const auto result = sendPacketWithReply(&packet); result.isError()) {
            return Result<uint8_t>::error(result.error());
        }
        return Result<uint8_t>::success(getByte());
    }

    Result<uint8_t> Visca::getRegister(const uint8_t reg_num) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_REGISTER_VALUE);
        appendByte(&packet, reg_num);

        if (const auto result = sendPacketWithReply(&packet); result.isError()) {
            return Result<uint8_t>::error(result.error());
        }
        const uint8_t reg_val = (transport_->iface.ibuf.at(2) << 4) + transport_->iface.ibuf.at(3);
        return Result<uint8_t>::success(reg_val);
    }

    /********************************/
    /* SPECIAL FUNCTIONS FOR D30/31 */
    /********************************/

    Result<void> Visca::setWideConLens(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_WIDE_CON_LENS);
        appendByte(&packet, VISCA_WIDE_CON_LENS_SET);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setAtModeOnoff() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_MODE);
        appendByte(&packet, VISCA_AT_ONOFF);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setAtMode(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_MODE);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setAtAeOnoff() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_AE);
        appendByte(&packet, VISCA_AT_ONOFF);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setAtAe(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_AE);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setAtAutozoomOnoff() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_AUTOZOOM);
        appendByte(&packet, VISCA_AT_ONOFF);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setAtAutozoom(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_AUTOZOOM);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setAtmdFramedisplayOnoff() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_ATMD_FRAMEDISPLAY);
        appendByte(&packet, VISCA_AT_ONOFF);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setAtmdFramedisplay(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_ATMD_FRAMEDISPLAY);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setAtFrameoffsetOnoff() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_FRAMEOFFSET);
        appendByte(&packet, VISCA_AT_ONOFF);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setAtFrameoffset(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_FRAMEOFFSET);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setAtmdStartstop() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_ATMD_STARTSTOP);
        appendByte(&packet, VISCA_AT_ONOFF);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setAtChase(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_CHASE);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setAtChaseNext() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_CHASE);
        appendByte(&packet, VISCA_AT_CHASE_NEXT);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setMdModeOnoff() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_MODE);
        appendByte(&packet, VISCA_MD_ONOFF);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setMdMode(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_MODE);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setMdFrame() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_FRAME);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setMdDetect() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_DETECT);
        appendByte(&packet, VISCA_MD_ONOFF);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setAtEntry(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_ENTRY);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setAtLostinfo() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_ATMD_LOSTINFO1);
        appendByte(&packet, VISCA_ATMD_LOSTINFO2);
        appendByte(&packet, VISCA_AT_LOSTINFO);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setMdLostinfo() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_ATMD_LOSTINFO1);
        appendByte(&packet, VISCA_ATMD_LOSTINFO2);
        appendByte(&packet, VISCA_MD_LOSTINFO);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setMdAdjustYlevel(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_YLEVEL);
        appendByte(&packet, VISCA_MD_ADJUST);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setMdAdjustHuelevel(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_HUELEVEL);
        appendByte(&packet, VISCA_MD_ADJUST);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setMdAdjustSize(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_SIZE);
        appendByte(&packet, VISCA_MD_ADJUST);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setMdAdjustDisptime(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_DISPTIME);
        appendByte(&packet, VISCA_MD_ADJUST);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setMdAdjustRefmode(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_REFMODE);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setMdAdjustReftime(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_REFTIME);
        appendByte(&packet, VISCA_MD_ADJUST);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setMdMeasureMode1Onoff() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_MEASURE_MODE_1);
        appendByte(&packet, VISCA_MD_ONOFF);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setMdMeasureMode1(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_MEASURE_MODE_1);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setMdMeasureMode2Onoff() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_MEASURE_MODE_2);
        appendByte(&packet, VISCA_MD_ONOFF);

        return sendPacketWithReply(&packet);
    }

    Result<void> Visca::setMdMeasureMode2(const uint8_t power) const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_MEASURE_MODE_2);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
    }

    Result<uint8_t> Visca::getKeylock() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_KEYLOCK);

        if (const auto result = sendPacketWithReply(&packet); result.isError()) {
            return Result<uint8_t>::error(result.error());
        }
        return Result<uint8_t>::success(getByte());
    }

    Result<uint8_t> Visca::getWideConLens() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_WIDE_CON_LENS);

        if (const auto result = sendPacketWithReply(&packet); result.isError()) {
            return Result<uint8_t>::error(result.error());
        }
        return Result<uint8_t>::success(getByte());
    }

    Result<uint8_t> Visca::getAtmdMode() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_ATMD_MODE);

        if (const auto result = sendPacketWithReply(&packet); result.isError()) {
            return Result<uint8_t>::error(result.error());
        }
        return Result<uint8_t>::success(getByte());
    }

    Result<uint16_t> Visca::getAtMode() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_MODE_QUERY);

        if (const auto result = sendPacketWithReply(&packet); result.isError()) {
            return Result<uint16_t>::error(result.error());
        }
        const uint16_t value = ((transport_->iface.ibuf.at(2) & 0xff) << 8) + (transport_->iface.ibuf.at(3) & 0xff);
        return Result<uint16_t>::success(value);
    }

    Result<uint8_t> Visca::getAtEntry() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_ENTRY);

        if (const auto result = sendPacketWithReply(&packet); result.isError()) {
            return Result<uint8_t>::error(result.error());
        }
        return Result<uint8_t>::success(getByte());
    }

    Result<uint16_t> Visca::getMdMode() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_MODE_QUERY);
        if (const auto result = sendPacketWithReply(&packet); result.isError()) {
            return Result<uint16_t>::error(result.error());
        }
        const uint16_t value = ((transport_->iface.ibuf.at(2) & 0xff) << 8) + (transport_->iface.ibuf.at(3) & 0xff);
        return Result<uint16_t>::success(value);
    }

    Result<uint8_t> Visca::getMdYlevel() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_YLEVEL);
        if (const auto result = sendPacketWithReply(&packet); result.isError()) {
            return Result<uint8_t>::error(result.error());
        }
        const uint8_t power = (transport_->iface.ibuf.at(3) & 0x0f);
        return Result<uint8_t>::success(power);
    }

    Result<uint8_t> Visca::getMdHuelevel() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_HUELEVEL);
        if (const auto result = sendPacketWithReply(&packet); result.isError()) {
            return Result<uint8_t>::error(result.error());
        }
        const uint8_t power = (transport_->iface.ibuf.at(3) & 0x0f);
        return Result<uint8_t>::success(power);
    }

    Result<uint8_t> Visca::getMdSize() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_SIZE);
        if (const auto result = sendPacketWithReply(&packet); result.isError()) {
            return Result<uint8_t>::error(result.error());
        }
        const uint8_t power = (transport_->iface.ibuf.at(3) & 0x0f);
        return Result<uint8_t>::success(power);
    }

    Result<uint8_t> Visca::getMdDisptime() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_DISPTIME);

        if (const auto result = sendPacketWithReply(&packet); result.isError()) {
            return Result<uint8_t>::error(result.error());
        }
        const uint8_t power = (transport_->iface.ibuf.at(3) & 0x0f);
        return Result<uint8_t>::success(power);
    }

    Result<uint8_t> Visca::getMdRefmode() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_REFMODE);

        if (const auto result = sendPacketWithReply(&packet); result.isError()) {
            return Result<uint8_t>::error(result.error());
        }
        return Result<uint8_t>::success(getByte());
    }

    Result<uint8_t> Visca::getMdReftime() const {
        ViscaPacket packet{};

        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_REFTIME_QUERY);

        if (const auto result = sendPacketWithReply(&packet); result.isError()) {
            return Result<uint8_t>::error(result.error());
        }
        const uint8_t power = (transport_->iface.ibuf.at(3) & 0x0f);
        return Result<uint8_t>::success(power);
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