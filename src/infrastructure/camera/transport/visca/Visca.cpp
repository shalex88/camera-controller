#include "Visca.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "infrastructure/camera/transport/uart/UartTransport.h"

namespace camera_service::infrastructure {
    ErrorCode Visca::sendPacket(ViscaPacket* packet) const {
        // check data:
        if ((transport_->address_ > 7) || (camera_.address > 7) || (transport_->broadcast_ > 1)) {
            return ErrorCode::Failure;
        }

        // build header:
        packet->data.at(0) = 0x80;
        packet->data.at(0) |= (transport_->address_ << 4);
        if (transport_->broadcast_ > 0) {
            packet->data.at(0) |= (transport_->broadcast_ << 3);
            packet->data.at(0) &= 0xF8;
        }
        else {
            packet->data.at(0) |= camera_.address;
        }

        // append footer
        appendByte(packet, VISCA_TERMINATOR);

        if (!transport_->write(packet->data) ) {
            return ErrorCode::Failure;
        }
        return ErrorCode::Success;
    }

    ErrorCode Visca::getPacket() const {
        transport_->read();
        return ErrorCode::Success;
    }

    /***********************************/
    /*       SYSTEM  FUNCTIONS         */
    /***********************************/

    ErrorCode Visca::connect() const {
        transport_->connect();
        return ErrorCode::Success;
    }

    ErrorCode Visca::disconnect() const {
        transport_->disconnect();
        return ErrorCode::Success;
    }

    void Visca::appendByte(ViscaPacket* packet, const uint8_t byte) {
        packet->data.at(packet->length) = byte;
        (packet->length)++;
    }

    void Visca::initPacket(ViscaPacket* packet) {
        // we start writing at byte 1, the first byte will be filled by the
        // packet sending function. This function will also append a terminator.
        packet->length = 1;
    }

    ErrorCode Visca::getReply() {
        // first message: -------------------
        if (getPacket() != ErrorCode::Success) {
            return ErrorCode::Failure;
        }
        type_ = static_cast<ResponseType>(transport_->ibuf_[1] & 0xF0);

        // skip ack messages
        while (type_ == ResponseType::Ack) {
            if (getPacket() != ErrorCode::Success) {
                return ErrorCode::Failure;
            }
            type_ = static_cast<ResponseType>(transport_->ibuf_[1] & 0xF0);
        }

        switch (type_) {
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

    ErrorCode Visca::sendPacketWithReply(ViscaPacket* packet) {
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

        ioctl(transport_->port_fd_, FIONREAD, &bytes);
        if (bytes > 0) {
            bytes = (bytes > *buffer_size) ? *buffer_size : bytes;
            read(transport_->port_fd_, &buffer, bytes);
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

    Visca::Visca(std::unique_ptr<UartTransport> transport): transport_(std::move(transport)) {
    }

    ErrorCode Visca::setAddress(int* camera_num) {
        ViscaPacket packet{};

        camera_.address = 0;
        const auto backup = transport_->broadcast_;

        initPacket(&packet);
        appendByte(&packet, 0x30);
        appendByte(&packet, 0x01);

        transport_->broadcast_ = 1;
        if (sendPacket(&packet) != ErrorCode::Success) {
            transport_->broadcast_ = backup;
            return ErrorCode::Failure;
        }
        transport_->broadcast_ = backup;

        if (getReply() != ErrorCode::Success) {
            return ErrorCode::Failure;
        }
        /* We parse the message from the camera here  */
        /* We expect to receive 4*camera_num bytes,
               every packet should be 88 30 0x FF, x being
               the camera id+1. The number of cams will thus be
               ibuf[bytes-2]-1  */
        if ((transport_->bytes_ & 0x3) != 0) {
            /* check multiple of 4 */
            return ErrorCode::Failure;
        }
        *camera_num = transport_->ibuf_[transport_->bytes_ - 2] - 1;
        if ((*camera_num == 0) || (*camera_num > 7)) {
            return ErrorCode::Failure;
        }
        return ErrorCode::Success;
    }

    ErrorCode Visca::clear() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, 0x01);
        appendByte(&packet, 0x00);
        appendByte(&packet, 0x01);

        if (sendPacket(&packet) != ErrorCode::Success) {
            return ErrorCode::Failure;
        }
        if (getReply() != ErrorCode::Success) {
            return ErrorCode::Failure;
        }
        return ErrorCode::Success;
    }

    ErrorCode Visca::getCameraInfo(ViscaCamera* camera) {
        ViscaPacket packet{};
        packet.data[0] = 0x80 | camera_.address;
        packet.data[1] = 0x09;
        packet.data[2] = 0x00;
        packet.data[3] = 0x02;
        packet.data[4] = VISCA_TERMINATOR;
        packet.length = 5;

        if (!transport_->write(packet.data)) {
            return ErrorCode::Failure;
        }
        if (getReply() != ErrorCode::Success) {
            return ErrorCode::Failure;
        }

        if (transport_->bytes_ != 10) {
            /* we expect 10 bytes as answer */
            return ErrorCode::Failure;
        }
        camera_.vendor = (transport_->ibuf_[2] << 8) + transport_->ibuf_[3];
        camera_.model = (transport_->ibuf_[4] << 8) + transport_->ibuf_[5];
        camera_.rom_version = (transport_->ibuf_[6] << 8) + transport_->ibuf_[7];
        camera_.socket_num = transport_->ibuf_[8];
        return ErrorCode::Success;
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

    ErrorCode Visca::setZoomValue(const uint32_t zoom) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ZOOM_VALUE);
        appendByte(&packet, (zoom & 0xF000) >> 12);
        appendByte(&packet, (zoom & 0x0F00) >> 8);
        appendByte(&packet, (zoom & 0x00F0) >> 4);
        appendByte(&packet, (zoom & 0x000F));

        return sendPacketWithReply(&packet);
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

    ErrorCode Visca::setFocusValue(const uint32_t focus) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_VALUE);
        appendByte(&packet, (focus & 0xF000) >> 12);
        appendByte(&packet, (focus & 0x0F00) >> 8);
        appendByte(&packet, (focus & 0x00F0) >> 4);
        appendByte(&packet, (focus & 0x000F));

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setFocusAuto(const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_AUTO);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
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

        return ErrorCode::Success;
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

    ErrorCode Visca::setCamStabilizer(const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_CAM_STABILIZER);
        appendByte(&packet, power);

        return sendPacketWithReply(&packet);
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
        appendByte(&packet, VISCA_SPOT_AE_ON);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setSpotAeOff() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_SPOT_AE);
        appendByte(&packet, VISCA_SPOT_AE_OFF);

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
        if (const ErrorCode err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *power = transport_->ibuf_[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getDzoom(uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DZOOM);
        if (const ErrorCode err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *power = transport_->ibuf_[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getDzoomLimit(uint8_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DZOOM_LIMIT);
        if (const ErrorCode err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *value = transport_->ibuf_[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getZoomValue(uint16_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ZOOM_VALUE);

        if (const ErrorCode err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *value = (transport_->ibuf_[2] << 12) + (transport_->ibuf_[3] << 8) + (transport_->ibuf_[4] << 4) + transport_->
            ibuf_[5];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getFocusAuto(uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_AUTO);

        if (const ErrorCode err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *power = transport_->ibuf_[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getFocusValue(uint16_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_VALUE);

        if (const ErrorCode err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *value = (transport_->ibuf_[2] << 12) + (transport_->ibuf_[3] << 8) + (transport_->ibuf_[4] << 4) + transport_->
            ibuf_[5];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getFocusAutoSense(uint8_t* mode) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_AUTO_SENSE);

        if (const ErrorCode err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *mode = transport_->ibuf_[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getFocusNearLimit(uint16_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_NEAR_LIMIT);

        if (const ErrorCode err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *value = (transport_->ibuf_[2] << 12) + (transport_->ibuf_[3] << 8) + (transport_->ibuf_[4] << 4) + transport_->
            ibuf_[5];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getWhitebalMode(uint8_t* mode) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_WB);

        if (const ErrorCode err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *mode = transport_->ibuf_[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getRgainValue(uint16_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_RGAIN_VALUE);

        if (const ErrorCode err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *value = (transport_->ibuf_[2] << 12) + (transport_->ibuf_[3] << 8) + (transport_->ibuf_[4] << 4) + transport_->
            ibuf_[5];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getBgainValue(uint16_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BGAIN_VALUE);

        if (const ErrorCode err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *value = (transport_->ibuf_[2] << 12) + (transport_->ibuf_[3] << 8) + (transport_->ibuf_[4] << 4) + transport_->
            ibuf_[5];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getAutoExpMode(uint8_t* mode) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_AUTO_EXP);

        if (const ErrorCode err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *mode = transport_->ibuf_[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getSlowShutterAuto(uint8_t* mode) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_SLOW_SHUTTER);

        if (const ErrorCode err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *mode = transport_->ibuf_[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getShutterValue(uint16_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_SHUTTER_VALUE);

        if (const ErrorCode err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *value = (transport_->ibuf_[2] << 12) + (transport_->ibuf_[3] << 8) + (transport_->ibuf_[4] << 4) + transport_->
            ibuf_[5];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getIrisValue(uint16_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_IRIS_VALUE);

        if (const ErrorCode err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *value = (transport_->ibuf_[2] << 12) + (transport_->ibuf_[3] << 8) + (transport_->ibuf_[4] << 4) + transport_->
            ibuf_[5];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getGainValue(uint16_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_GAIN_VALUE);

        if (const ErrorCode err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *value = (transport_->ibuf_[2] << 12) + (transport_->ibuf_[3] << 8) + (transport_->ibuf_[4] << 4) + transport_->
            ibuf_[5];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getBrightValue(uint16_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BRIGHT_VALUE);

        if (const ErrorCode err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *value = (transport_->ibuf_[2] << 12) + (transport_->ibuf_[3] << 8) + (transport_->ibuf_[4] << 4) + transport_->
            ibuf_[5];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getExpCompPower(uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_EXP_COMP_POWER);

        if (const ErrorCode err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *power = transport_->ibuf_[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getExpCompValue(uint16_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_EXP_COMP_VALUE);

        if (const ErrorCode err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *value = (transport_->ibuf_[2] << 12) + (transport_->ibuf_[3] << 8) + (transport_->ibuf_[4] << 4) + transport_->
            ibuf_[5];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getBacklightComp(uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BACKLIGHT_COMP);

        if (const ErrorCode err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *power = transport_->ibuf_[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getApertureValue(uint16_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_APERTURE_VALUE);

        if (const ErrorCode err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *value = (transport_->ibuf_[2] << 12) + (transport_->ibuf_[3] << 8) + (transport_->ibuf_[4] << 4) + transport_->
            ibuf_[5];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getZeroLuxShot(uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ZERO_LUX);

        if (const ErrorCode err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *power = transport_->ibuf_[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getIrLed(uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_IR_LED);

        if (const ErrorCode err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *power = transport_->ibuf_[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getWideMode(uint8_t* mode) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_WIDE_MODE);

        if (const ErrorCode err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *mode = transport_->ibuf_[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getMirror(uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_MIRROR);

        if (const ErrorCode err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *power = transport_->ibuf_[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getFreeze(uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FREEZE);

        if (const ErrorCode err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *power = transport_->ibuf_[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getPictureEffect(uint8_t* mode) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_PICTURE_EFFECT);

        if (const ErrorCode err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *mode = transport_->ibuf_[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getDigitalEffect(uint8_t* mode) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DIGITAL_EFFECT);

        if (const ErrorCode err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *mode = transport_->ibuf_[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getDigitalEffectLevel(uint16_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DIGITAL_EFFECT_LEVEL);

        if (const ErrorCode err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *value = (transport_->ibuf_[2] << 12) + (transport_->ibuf_[3] << 8) + (transport_->ibuf_[4] << 4) + transport_->
            ibuf_[5];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getMemory(uint8_t* channel) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_MEMORY);

        if (const ErrorCode err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *channel = transport_->ibuf_[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getDisplay(uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DISPLAY);

        if (const ErrorCode err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *power = transport_->ibuf_[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getId(uint16_t* id) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ID);

        if (const ErrorCode err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *id = (transport_->ibuf_[2] << 12) + (transport_->ibuf_[3] << 8) + (transport_->ibuf_[4] << 4) + transport_->
            ibuf_[5];
        return ErrorCode::Success;
    }

    ErrorCode Visca::setIrreceiveOn() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_IRRECEIVE);
        appendByte(&packet, VISCA_IRRECEIVE_ON);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setIrreceiveOff() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_IRRECEIVE);
        appendByte(&packet, VISCA_IRRECEIVE_OFF);

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

    ErrorCode Visca::setPantiltAbsolutePosition(const ViscaCamera* camera, const uint32_t pan_speed,
                                                const uint32_t tilt_speed, const uint32_t pan_pos,
                                                const uint32_t tilt_pos) {
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

    ErrorCode Visca::setPantiltRelativePosition(const ViscaCamera* camera, const uint32_t pan_speed,
                                                const uint32_t tilt_speed, const uint32_t pan_pos,
                                                const uint32_t tilt_pos) {
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
        appendByte(&packet, VISCA_PT_DATASCREEN_ON);

        return sendPacketWithReply(&packet);
    }

    ErrorCode Visca::setDatascreenOff() {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_DATASCREEN);
        appendByte(&packet, VISCA_PT_DATASCREEN_OFF);

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

        if (const ErrorCode err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *system = transport_->ibuf_[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getPantiltMode(uint16_t* status) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_MODE_INQ);

        if (const ErrorCode err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *status = ((transport_->ibuf_[2] & 0xff) << 8) + (transport_->ibuf_[3] & 0xff);
        return ErrorCode::Success;
    }

    ErrorCode Visca::getPantiltMaxspeed(uint8_t* max_pan_speed, uint8_t* max_tilt_speed) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_MAXSPEED_INQ);

        if (const ErrorCode err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *max_pan_speed = (transport_->ibuf_[2] & 0xff);
        *max_tilt_speed = (transport_->ibuf_[3] & 0xff);
        return ErrorCode::Success;
    }

    ErrorCode Visca::getPantiltPosition(uint16_t* pan_position, uint16_t* tilt_position) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_POSITION_INQ);

        if (const ErrorCode err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *pan_position = ((transport_->ibuf_[2] & 0xf) << 12) + ((transport_->ibuf_[3] & 0xf) << 8) + ((transport_->ibuf_
            [4] & 0xf) << 4) + (transport_->ibuf_[5] & 0xf);
        *tilt_position = ((transport_->ibuf_[6] & 0xf) << 12) + ((transport_->ibuf_[7] & 0xf) << 8) + ((transport_->
            ibuf_[8] & 0xf) << 4) + (transport_->ibuf_[9] & 0xf);

        return ErrorCode::Success;
    }

    ErrorCode Visca::getDatascreen(uint8_t* status) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_DATASCREEN_INQ);

        if (const ErrorCode err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *status = transport_->ibuf_[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getRegister(const uint8_t reg_num, uint8_t* reg_val) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_REGISTER_VALUE);
        appendByte(&packet, reg_num);

        if (const ErrorCode err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *reg_val = (transport_->ibuf_[2] << 4) + transport_->ibuf_[3];
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

        if (const ErrorCode err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *power = transport_->ibuf_[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getWideConLens(uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_WIDE_CON_LENS);

        if (const ErrorCode err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *power = transport_->ibuf_[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getAtmdMode(uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_ATMD_MODE);

        if (const ErrorCode err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *power = transport_->ibuf_[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getAtMode(uint16_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_MODE_QUERY);

        if (const ErrorCode err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *value = ((transport_->ibuf_[2] & 0xff) << 8) + (transport_->ibuf_[3] & 0xff);
        return ErrorCode::Success;
    }

    ErrorCode Visca::getAtEntry(uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_ENTRY);

        if (const ErrorCode err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *power = transport_->ibuf_[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getMdMode(uint16_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_MODE_QUERY);
        if (const ErrorCode err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *value = ((transport_->ibuf_[2] & 0xff) << 8) + (transport_->ibuf_[3] & 0xff);
        return ErrorCode::Success;
    }

    ErrorCode Visca::getMdYlevel(uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_YLEVEL);
        if (const ErrorCode err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *power = (transport_->ibuf_[3] & 0x0f);
        return ErrorCode::Success;
    }

    ErrorCode Visca::getMdHuelevel(uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_HUELEVEL);
        if (const ErrorCode err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *power = (transport_->ibuf_[3] & 0x0f);
        return ErrorCode::Success;
    }

    ErrorCode Visca::getMdSize(uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_SIZE);
        if (const ErrorCode err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *power = (transport_->ibuf_[3] & 0x0f);
        return ErrorCode::Success;
    }

    ErrorCode Visca::getMdDisptime(uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_DISPTIME);

        if (const ErrorCode err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *power = (transport_->ibuf_[3] & 0x0f);
        return ErrorCode::Success;
    }

    ErrorCode Visca::getMdRefmode(uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_REFMODE);

        if (const ErrorCode err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *power = transport_->ibuf_[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getMdReftime(uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_REFTIME_QUERY);

        if (const ErrorCode err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *power = (transport_->ibuf_[3] & 0x0f);
        return ErrorCode::Success;
    }

    ErrorCode Visca::getAtObjPos(uint8_t* xpos, uint8_t* ypos, uint8_t* status) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_POSITION);

        if (const ErrorCode err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *xpos = transport_->ibuf_[2];
        *ypos = transport_->ibuf_[3];
        *status = (transport_->ibuf_[4] & 0x0f);
        return ErrorCode::Success;
    }

    ErrorCode Visca::getMdObjPos(uint8_t* xpos, uint8_t* ypos, uint8_t* status) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_POSITION);

        if (const ErrorCode err = sendPacketWithReply(&packet); err != ErrorCode::Success) {
            return err;
        }
        *xpos = transport_->ibuf_[2];
        *ypos = transport_->ibuf_[3];
        *status = (transport_->ibuf_[4] & 0x0f);
        return ErrorCode::Success;
    }
}
