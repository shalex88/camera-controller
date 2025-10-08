#include "Visca.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

namespace camera_service::infrastructure {
    ErrorCode Visca::writePacketData(const ViscaInterface* iface, const ViscaPacket* packet) {
        if (const auto err = write(iface->port_fd, packet->bytes, packet->length); err < packet->length) {
            return ErrorCode::Failure;
        }
        return ErrorCode::Success;
    }

    ErrorCode Visca::sendPacket(const ViscaInterface* iface, const ViscaCamera* camera, ViscaPacket* packet) {
        // check data:
        if ((iface->address > 7) || (camera->address > 7) || (iface->broadcast > 1)) {
            return ErrorCode::Failure;
        }

        // build header:
        packet->bytes[0] = 0x80;
        packet->bytes[0] |= (iface->address << 4);
        if (iface->broadcast > 0) {
            packet->bytes[0] |= (iface->broadcast << 3);
            packet->bytes[0] &= 0xF8;
        } else {
            packet->bytes[0] |= camera->address;
        }

        // append footer
        appendByte(packet, VISCA_TERMINATOR);

        return writePacketData(iface, packet);
    }

    ErrorCode Visca::getPacket(ViscaInterface* iface) {
        int pos = 0;

        // wait for message
        ioctl(iface->port_fd, FIONREAD, &(iface->bytes));
        while (iface->bytes == 0) {
            usleep(0);
            ioctl(iface->port_fd, FIONREAD, &(iface->bytes));
        }

        // get octets one by one
        read(iface->port_fd, iface->ibuf, 1);
        while (iface->ibuf[pos] != VISCA_TERMINATOR) {
            pos++;
            read(iface->port_fd, &iface->ibuf[pos], 1);
            usleep(0);
        }
        iface->bytes = pos + 1;

        return ErrorCode::Success;
    }

    /***********************************/
    /*       SYSTEM  FUNCTIONS         */
    /***********************************/

    ErrorCode Visca::openSerial(ViscaInterface* iface, const char* device_name) {
        const auto fd = open(device_name, O_RDWR | O_NDELAY | O_NOCTTY);

        if (fd == -1) {
            iface->port_fd = -1;
            return ErrorCode::Failure;
        }
        fcntl(fd, F_SETFL, 0);
        /* Setting port parameters */
        tcgetattr(fd, &iface->options);

        /* control flags */
        cfsetispeed(&iface->options,B9600); /* 9600 Bds   */
        iface->options.c_cflag &= ~PARENB; /* No parity  */
        iface->options.c_cflag &= ~CSTOPB; /*            */
        iface->options.c_cflag &= ~CSIZE; /* 8bit       */
        iface->options.c_cflag |= CS8; /*            */
        iface->options.c_cflag &= ~CRTSCTS; /* No hdw ctl */

        /* local flags */
        iface->options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); /* raw input */

        /* input flags */
        /*
            iface->options.c_iflag &= ~(INPCK | ISTRIP); // no parity
            iface->options.c_iflag &= ~(IXON | IXOFF | IXANY); // no soft ctl
            */
        /* patch: bpflegin: set to 0 in order to avoid invalid pan/tilt return values */
        iface->options.c_iflag = 0;

        /* output flags */
        iface->options.c_oflag &= ~OPOST; /* raw output */

        tcsetattr(fd, TCSANOW, &iface->options);
        iface->port_fd = fd;
        iface->address = 0;

        return ErrorCode::Success;
    }

    ErrorCode Visca::closeSerial(ViscaInterface* iface) {
        if (iface->port_fd != -1) {
            close(iface->port_fd);
            iface->port_fd = -1;
            return ErrorCode::Success;
        }
        return ErrorCode::Failure;
    }

    void Visca::appendByte(ViscaPacket* packet, const uint8_t byte) {
        packet->bytes[packet->length] = byte;
        (packet->length)++;
    }

    void Visca::initPacket(ViscaPacket* packet) {
        // we start writing at byte 1, the first byte will be filled by the
        // packet sending function. This function will also append a terminator.
        packet->length = 1;
    }

    ErrorCode Visca::getReply(ViscaInterface* iface) {
        // first message: -------------------
        if (getPacket(iface) != ErrorCode::Success) {
            return ErrorCode::Failure;
        }
        iface->type = static_cast<ResponseType>(iface->ibuf[1] & 0xF0);

        // skip ack messages
        while (iface->type == ResponseType::Ack) {
            if (getPacket(iface) != ErrorCode::Success) {
                return ErrorCode::Failure;
            }
            iface->type = static_cast<ResponseType>(iface->ibuf[1] & 0xF0);
        }

        switch (iface->type) {
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

    ErrorCode Visca::sendPacketWithReply(ViscaInterface* iface, const ViscaCamera* camera, ViscaPacket* packet) {
        if (sendPacket(iface, camera, packet) != ErrorCode::Success) {
            return ErrorCode::Failure;
        }

        if (getReply(iface) != ErrorCode::Success) {
            return ErrorCode::Failure;
        }

        return ErrorCode::Success;
    }

    ErrorCode Visca::unreadBytes(const ViscaInterface* iface, unsigned char* buffer, uint32_t* buffer_size) {
        uint32_t bytes = 0;
        *buffer_size = 0;

        ioctl(iface->port_fd, FIONREAD, &bytes);
        if (bytes > 0) {
            bytes = (bytes > *buffer_size) ? *buffer_size : bytes;
            read(iface->port_fd, &buffer, bytes);
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

    ErrorCode Visca::setAddress(ViscaInterface* iface, int* camera_num) {
        ViscaPacket packet{};
        ViscaCamera camera{};

        camera.address = 0;
        const auto backup = iface->broadcast;

        initPacket(&packet);
        appendByte(&packet, 0x30);
        appendByte(&packet, 0x01);

        iface->broadcast = 1;
        if (sendPacket(iface, &camera, &packet) != ErrorCode::Success) {
            iface->broadcast = backup;
            return ErrorCode::Failure;
        }
        iface->broadcast = backup;

        if (getReply(iface) != ErrorCode::Success) {
            return ErrorCode::Failure;
        }
        /* We parse the message from the camera here  */
        /* We expect to receive 4*camera_num bytes,
               every packet should be 88 30 0x FF, x being
               the camera id+1. The number of cams will thus be
               ibuf[bytes-2]-1  */
        if ((iface->bytes & 0x3) != 0) {
            /* check multiple of 4 */
            return ErrorCode::Failure;
        }
        *camera_num = iface->ibuf[iface->bytes - 2] - 1;
        if ((*camera_num == 0) || (*camera_num > 7)) {
            return ErrorCode::Failure;
        }
        return ErrorCode::Success;
    }

    ErrorCode Visca::clear(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, 0x01);
        appendByte(&packet, 0x00);
        appendByte(&packet, 0x01);

        if (sendPacket(iface, camera, &packet) != ErrorCode::Success) {
            return ErrorCode::Failure;
        }
        if (getReply(iface) != ErrorCode::Success) {
            return ErrorCode::Failure;
        }
        return ErrorCode::Success;
    }

    ErrorCode Visca::getCameraInfo(ViscaInterface* iface, ViscaCamera* camera) {
        ViscaPacket packet{};
        packet.bytes[0] = 0x80 | camera->address;
        packet.bytes[1] = 0x09;
        packet.bytes[2] = 0x00;
        packet.bytes[3] = 0x02;
        packet.bytes[4] = VISCA_TERMINATOR;
        packet.length = 5;

        if (writePacketData(iface, &packet) != ErrorCode::Success) {
            return ErrorCode::Failure;
        }
        if (getReply(iface) != ErrorCode::Success) {
            return ErrorCode::Failure;
        }

        if (iface->bytes != 10) {
            /* we expect 10 bytes as answer */
            return ErrorCode::Failure;
        }
        camera->vendor = (iface->ibuf[2] << 8) + iface->ibuf[3];
        camera->model = (iface->ibuf[4] << 8) + iface->ibuf[5];
        camera->rom_version = (iface->ibuf[6] << 8) + iface->ibuf[7];
        camera->socket_num = iface->ibuf[8];
        return ErrorCode::Success;
    }

    /***********************************/
    /*       COMMAND FUNCTIONS         */
    /***********************************/

    ErrorCode Visca::setPower(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_POWER);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setKeylock(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_KEYLOCK);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setCameraId(ViscaInterface* iface, const ViscaCamera* camera, const uint16_t id) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ID);
        appendByte(&packet, (id & 0xF000) >> 12);
        appendByte(&packet, (id & 0x0F00) >> 8);
        appendByte(&packet, (id & 0x00F0) >> 4);
        appendByte(&packet, (id & 0x000F));

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setZoomTele(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ZOOM);
        appendByte(&packet, VISCA_ZOOM_TELE);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setZoomWide(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ZOOM);
        appendByte(&packet, VISCA_ZOOM_WIDE);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setZoomStop(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ZOOM);
        appendByte(&packet, VISCA_ZOOM_STOP);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setZoomTeleSpeed(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t speed) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ZOOM);
        appendByte(&packet, VISCA_ZOOM_TELE_SPEED | (speed & 0x7));

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setZoomWideSpeed(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t speed) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ZOOM);
        appendByte(&packet, VISCA_ZOOM_WIDE_SPEED | (speed & 0x7));

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setZoomValue(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t zoom) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ZOOM_VALUE);
        appendByte(&packet, (zoom & 0xF000) >> 12);
        appendByte(&packet, (zoom & 0x0F00) >> 8);
        appendByte(&packet, (zoom & 0x00F0) >> 4);
        appendByte(&packet, (zoom & 0x000F));

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setZoomAndFocusValue(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t zoom,
                                           const uint32_t focus) {
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

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setDzoom(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DZOOM);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setDzoomLimit(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t limit) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DZOOM_LIMIT);
        appendByte(&packet, limit);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setDzoomMode(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DZOOM_MODE);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setFocusFar(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS);
        appendByte(&packet, VISCA_FOCUS_FAR);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setFocusNear(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS);
        appendByte(&packet, VISCA_FOCUS_NEAR);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setFocusStop(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS);
        appendByte(&packet, VISCA_FOCUS_STOP);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setFocusFarSpeed(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t speed) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS);
        appendByte(&packet, VISCA_FOCUS_FAR_SPEED | (speed & 0x7));

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setFocusNearSpeed(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t speed) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS);
        appendByte(&packet, VISCA_FOCUS_NEAR_SPEED | (speed & 0x7));

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setFocusValue(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t focus) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_VALUE);
        appendByte(&packet, (focus & 0xF000) >> 12);
        appendByte(&packet, (focus & 0x0F00) >> 8);
        appendByte(&packet, (focus & 0x00F0) >> 4);
        appendByte(&packet, (focus & 0x000F));

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setFocusAuto(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_AUTO);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setFocusOnePush(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_ONE_PUSH);
        appendByte(&packet, VISCA_FOCUS_ONE_PUSH_TRIG);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setFocusInfinity(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_ONE_PUSH);
        appendByte(&packet, VISCA_FOCUS_ONE_PUSH_INF);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setFocusAutosenseHigh(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_AUTO_SENSE);
        appendByte(&packet, VISCA_FOCUS_AUTO_SENSE_HIGH);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setFocusAutosenseLow(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_AUTO_SENSE);
        appendByte(&packet, VISCA_FOCUS_AUTO_SENSE_LOW);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setFocusNearLimit(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t limit) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_NEAR_LIMIT);
        appendByte(&packet, (limit & 0xF000) >> 12);
        appendByte(&packet, (limit & 0x0F00) >> 8);
        appendByte(&packet, (limit & 0x00F0) >> 4);
        appendByte(&packet, (limit & 0x000F));

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setWhitebalMode(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t mode) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_WB);
        appendByte(&packet, mode);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setWhitebalOnePush(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_WB_TRIGGER);
        appendByte(&packet, VISCA_WB_ONE_PUSH_TRIG);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setRgainUp(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_RGAIN);
        appendByte(&packet, VISCA_UP);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setRgainDown(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_RGAIN);
        appendByte(&packet, VISCA_DOWN);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setRgainReset(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_RGAIN);
        appendByte(&packet, VISCA_RESET);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setRgainValue(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_RGAIN_VALUE);
        appendByte(&packet, (value & 0xF000) >> 12);
        appendByte(&packet, (value & 0x0F00) >> 8);
        appendByte(&packet, (value & 0x00F0) >> 4);
        appendByte(&packet, (value & 0x000F));

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setBgainUp(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BGAIN);
        appendByte(&packet, VISCA_UP);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setBgainDown(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BGAIN);
        appendByte(&packet, VISCA_DOWN);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setBgainReset(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BGAIN);
        appendByte(&packet, VISCA_RESET);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setBgainValue(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BGAIN_VALUE);
        appendByte(&packet, (value & 0xF000) >> 12);
        appendByte(&packet, (value & 0x0F00) >> 8);
        appendByte(&packet, (value & 0x00F0) >> 4);
        appendByte(&packet, (value & 0x000F));

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setShutterUp(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_SHUTTER);
        appendByte(&packet, VISCA_UP);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setShutterDown(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_SHUTTER);
        appendByte(&packet, VISCA_DOWN);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setShutterReset(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_SHUTTER);
        appendByte(&packet, VISCA_RESET);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setShutterValue(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_SHUTTER_VALUE);
        appendByte(&packet, (value & 0xF000) >> 12);
        appendByte(&packet, (value & 0x0F00) >> 8);
        appendByte(&packet, (value & 0x00F0) >> 4);
        appendByte(&packet, (value & 0x000F));

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setIrisUp(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_IRIS);
        appendByte(&packet, VISCA_UP);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setIrisDown(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_IRIS);
        appendByte(&packet, VISCA_DOWN);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setIrisReset(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_IRIS);
        appendByte(&packet, VISCA_RESET);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setIrisValue(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_IRIS_VALUE);
        appendByte(&packet, (value & 0xF000) >> 12);
        appendByte(&packet, (value & 0x0F00) >> 8);
        appendByte(&packet, (value & 0x00F0) >> 4);
        appendByte(&packet, (value & 0x000F));

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setGainUp(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_GAIN);
        appendByte(&packet, VISCA_UP);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setGainDown(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_GAIN);
        appendByte(&packet, VISCA_DOWN);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setGainReset(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_GAIN);
        appendByte(&packet, VISCA_RESET);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setGainValue(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_GAIN_VALUE);
        appendByte(&packet, (value & 0xF000) >> 12);
        appendByte(&packet, (value & 0x0F00) >> 8);
        appendByte(&packet, (value & 0x00F0) >> 4);
        appendByte(&packet, (value & 0x000F));

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setBrightUp(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BRIGHT);
        appendByte(&packet, VISCA_UP);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setBrightDown(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BRIGHT);
        appendByte(&packet, VISCA_DOWN);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setBrightReset(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BRIGHT);
        appendByte(&packet, VISCA_RESET);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setBrightValue(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BRIGHT_VALUE);
        appendByte(&packet, (value & 0xF000) >> 12);
        appendByte(&packet, (value & 0x0F00) >> 8);
        appendByte(&packet, (value & 0x00F0) >> 4);
        appendByte(&packet, (value & 0x000F));

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setApertureUp(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_APERTURE);
        appendByte(&packet, VISCA_UP);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setApertureDown(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_APERTURE);
        appendByte(&packet, VISCA_DOWN);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setApertureReset(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_APERTURE);
        appendByte(&packet, VISCA_RESET);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setApertureValue(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_APERTURE_VALUE);
        appendByte(&packet, (value & 0xF000) >> 12);
        appendByte(&packet, (value & 0x0F00) >> 8);
        appendByte(&packet, (value & 0x00F0) >> 4);
        appendByte(&packet, (value & 0x000F));

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setExpCompUp(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_EXP_COMP);
        appendByte(&packet, VISCA_UP);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setExpCompDown(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_EXP_COMP);
        appendByte(&packet, VISCA_DOWN);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setExpCompReset(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_EXP_COMP);
        appendByte(&packet, VISCA_RESET);

        return sendPacketWithReply(iface, camera, &packet);

        return ErrorCode::Success;
    }

    ErrorCode Visca::setExpCompValue(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_EXP_COMP_VALUE);
        appendByte(&packet, (value & 0xF000) >> 12);
        appendByte(&packet, (value & 0x0F00) >> 8);
        appendByte(&packet, (value & 0x00F0) >> 4);
        appendByte(&packet, (value & 0x000F));

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setExpCompPower(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_EXP_COMP_POWER);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setAutoExpMode(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t mode) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_AUTO_EXP);
        appendByte(&packet, mode);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setSlowShutterAuto(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_SLOW_SHUTTER);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setBacklightComp(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BACKLIGHT_COMP);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setZeroLuxShot(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ZERO_LUX);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setIrLed(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_IR_LED);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setWideMode(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t mode) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_WIDE_MODE);
        appendByte(&packet, mode);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setMirror(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_MIRROR);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setFreeze(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FREEZE);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setPictureEffect(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t mode) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_PICTURE_EFFECT);
        appendByte(&packet, mode);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setDigitalEffect(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t mode) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DIGITAL_EFFECT);
        appendByte(&packet, mode);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setDigitalEffectLevel(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t level) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DIGITAL_EFFECT_LEVEL);
        appendByte(&packet, level);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setCamStabilizer(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_CAM_STABILIZER);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::memorySet(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t channel) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_MEMORY);
        appendByte(&packet, VISCA_MEMORY_SET);
        appendByte(&packet, channel);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::memoryRecall(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t channel) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_MEMORY);
        appendByte(&packet, VISCA_MEMORY_RECALL);
        appendByte(&packet, channel);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::memoryReset(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t channel) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_MEMORY);
        appendByte(&packet, VISCA_MEMORY_RESET);
        appendByte(&packet, channel);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setDisplay(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DISPLAY);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setDateTime(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t year,
                                  const uint32_t month, const uint32_t day, const uint32_t hour,
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

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setDateDisplay(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DATE_DISPLAY);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setTimeDisplay(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_TIME_DISPLAY);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setTitleDisplay(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_TITLE_DISPLAY);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setTitleClear(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_TITLE_DISPLAY);
        appendByte(&packet, VISCA_TITLE_DISPLAY_CLEAR);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setTitleParams(ViscaInterface* iface, const ViscaCamera* camera, const ViscaTitleData* title) {
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

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setTitle(ViscaInterface* iface, const ViscaCamera* camera, const ViscaTitleData* title) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_TITLE_SET);
        appendByte(&packet, VISCA_TITLE_SET_PART1);

        for (auto i = 0; i < 10; i++) {
            appendByte(&packet, title->title[i]);
        }

        if (sendPacketWithReply(iface, camera, &packet) != ErrorCode::Success) {
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

        if (sendPacketWithReply(iface, camera, &packet) != ErrorCode::Success) {
            return ErrorCode::Failure;
        }

        return ErrorCode::Success;
    }

    ErrorCode Visca::setSpotAeOn(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_SPOT_AE);
        appendByte(&packet, VISCA_SPOT_AE_ON);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setSpotAeOff(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_SPOT_AE);
        appendByte(&packet, VISCA_SPOT_AE_OFF);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setSpotAePosition(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t x_position,
                                        const uint8_t y_position) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_SPOT_AE_POSITION);
        appendByte(&packet, (x_position & 0xF0) >> 4);
        appendByte(&packet, (x_position & 0x0F));
        appendByte(&packet, (y_position & 0xF0) >> 4);
        appendByte(&packet, (y_position & 0x0F));

        return sendPacketWithReply(iface, camera, &packet);
    }

    /***********************************/
    /*       INQUIRY FUNCTIONS         */
    /***********************************/

    ErrorCode Visca::getPower(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_POWER);
        if (const ErrorCode err = sendPacketWithReply(iface, camera, &packet); err != ErrorCode::Success) {
            return err;
        }
        *power = iface->ibuf[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getDzoom(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DZOOM);
        if (const ErrorCode err = sendPacketWithReply(iface, camera, &packet); err != ErrorCode::Success) {
            return err;
        }
        *power = iface->ibuf[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getDzoomLimit(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DZOOM_LIMIT);
        if (const ErrorCode err = sendPacketWithReply(iface, camera, &packet); err != ErrorCode::Success) {
            return err;
        }
        *value = iface->ibuf[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getZoomValue(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ZOOM_VALUE);

        if (const ErrorCode err = sendPacketWithReply(iface, camera, &packet); err != ErrorCode::Success) {
            return err;
        }
        *value = (iface->ibuf[2] << 12) + (iface->ibuf[3] << 8) + (iface->ibuf[4] << 4) + iface->ibuf[5];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getFocusAuto(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_AUTO);

        if (const ErrorCode err = sendPacketWithReply(iface, camera, &packet); err != ErrorCode::Success) {
            return err;
        }
        *power = iface->ibuf[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getFocusValue(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_VALUE);

        if (const ErrorCode err = sendPacketWithReply(iface, camera, &packet); err != ErrorCode::Success) {
            return err;
        }
        *value = (iface->ibuf[2] << 12) + (iface->ibuf[3] << 8) + (iface->ibuf[4] << 4) + iface->ibuf[5];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getFocusAutoSense(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* mode) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_AUTO_SENSE);

        if (const ErrorCode err = sendPacketWithReply(iface, camera, &packet); err != ErrorCode::Success) {
            return err;
        }
        *mode = iface->ibuf[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getFocusNearLimit(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_NEAR_LIMIT);

        if (const ErrorCode err = sendPacketWithReply(iface, camera, &packet); err != ErrorCode::Success) {
            return err;
        }
        *value = (iface->ibuf[2] << 12) + (iface->ibuf[3] << 8) + (iface->ibuf[4] << 4) + iface->ibuf[5];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getWhitebalMode(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* mode) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_WB);

        if (const ErrorCode err = sendPacketWithReply(iface, camera, &packet); err != ErrorCode::Success) {
            return err;
        }
        *mode = iface->ibuf[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getRgainValue(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_RGAIN_VALUE);

        if (const ErrorCode err = sendPacketWithReply(iface, camera, &packet); err != ErrorCode::Success) {
            return err;
        }
        *value = (iface->ibuf[2] << 12) + (iface->ibuf[3] << 8) + (iface->ibuf[4] << 4) + iface->ibuf[5];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getBgainValue(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BGAIN_VALUE);

        if (const ErrorCode err = sendPacketWithReply(iface, camera, &packet); err != ErrorCode::Success) {
            return err;
        }
        *value = (iface->ibuf[2] << 12) + (iface->ibuf[3] << 8) + (iface->ibuf[4] << 4) + iface->ibuf[5];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getAutoExpMode(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* mode) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_AUTO_EXP);

        if (const ErrorCode err = sendPacketWithReply(iface, camera, &packet); err != ErrorCode::Success) {
            return err;
        }
        *mode = iface->ibuf[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getSlowShutterAuto(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* mode) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_SLOW_SHUTTER);

        if (const ErrorCode err = sendPacketWithReply(iface, camera, &packet); err != ErrorCode::Success) {
            return err;
        }
        *mode = iface->ibuf[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getShutterValue(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_SHUTTER_VALUE);

        if (const ErrorCode err = sendPacketWithReply(iface, camera, &packet); err != ErrorCode::Success) {
            return err;
        }
        *value = (iface->ibuf[2] << 12) + (iface->ibuf[3] << 8) + (iface->ibuf[4] << 4) + iface->ibuf[5];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getIrisValue(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_IRIS_VALUE);

        if (const ErrorCode err = sendPacketWithReply(iface, camera, &packet); err != ErrorCode::Success) {
            return err;
        }
        *value = (iface->ibuf[2] << 12) + (iface->ibuf[3] << 8) + (iface->ibuf[4] << 4) + iface->ibuf[5];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getGainValue(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_GAIN_VALUE);

        if (const ErrorCode err = sendPacketWithReply(iface, camera, &packet); err != ErrorCode::Success) {
            return err;
        }
        *value = (iface->ibuf[2] << 12) + (iface->ibuf[3] << 8) + (iface->ibuf[4] << 4) + iface->ibuf[5];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getBrightValue(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BRIGHT_VALUE);

        if (const ErrorCode err = sendPacketWithReply(iface, camera, &packet); err != ErrorCode::Success) {
            return err;
        }
        *value = (iface->ibuf[2] << 12) + (iface->ibuf[3] << 8) + (iface->ibuf[4] << 4) + iface->ibuf[5];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getExpCompPower(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_EXP_COMP_POWER);

        if (const ErrorCode err = sendPacketWithReply(iface, camera, &packet); err != ErrorCode::Success) {
            return err;
        }
        *power = iface->ibuf[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getExpCompValue(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_EXP_COMP_VALUE);

        if (const ErrorCode err = sendPacketWithReply(iface, camera, &packet); err != ErrorCode::Success) {
            return err;
        }
        *value = (iface->ibuf[2] << 12) + (iface->ibuf[3] << 8) + (iface->ibuf[4] << 4) + iface->ibuf[5];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getBacklightComp(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BACKLIGHT_COMP);

        if (const ErrorCode err = sendPacketWithReply(iface, camera, &packet); err != ErrorCode::Success) {
            return err;
        }
        *power = iface->ibuf[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getApertureValue(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_APERTURE_VALUE);

        if (const ErrorCode err = sendPacketWithReply(iface, camera, &packet); err != ErrorCode::Success) {
            return err;
        }
        *value = (iface->ibuf[2] << 12) + (iface->ibuf[3] << 8) + (iface->ibuf[4] << 4) + iface->ibuf[5];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getZeroLuxShot(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ZERO_LUX);

        if (const ErrorCode err = sendPacketWithReply(iface, camera, &packet); err != ErrorCode::Success) {
            return err;
        }
        *power = iface->ibuf[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getIrLed(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_IR_LED);

        if (const ErrorCode err = sendPacketWithReply(iface, camera, &packet); err != ErrorCode::Success) {
            return err;
        }
        *power = iface->ibuf[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getWideMode(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* mode) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_WIDE_MODE);

        if (const ErrorCode err = sendPacketWithReply(iface, camera, &packet); err != ErrorCode::Success) {
            return err;
        }
        *mode = iface->ibuf[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getMirror(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_MIRROR);

        if (const ErrorCode err = sendPacketWithReply(iface, camera, &packet); err != ErrorCode::Success) {
            return err;
        }
        *power = iface->ibuf[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getFreeze(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FREEZE);

        if (const ErrorCode err = sendPacketWithReply(iface, camera, &packet); err != ErrorCode::Success) {
            return err;
        }
        *power = iface->ibuf[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getPictureEffect(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* mode) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_PICTURE_EFFECT);

        if (const ErrorCode err = sendPacketWithReply(iface, camera, &packet); err != ErrorCode::Success) {
            return err;
        }
        *mode = iface->ibuf[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getDigitalEffect(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* mode) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DIGITAL_EFFECT);

        if (const ErrorCode err = sendPacketWithReply(iface, camera, &packet); err != ErrorCode::Success) {
            return err;
        }
        *mode = iface->ibuf[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getDigitalEffectLevel(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DIGITAL_EFFECT_LEVEL);

        if (const ErrorCode err = sendPacketWithReply(iface, camera, &packet); err != ErrorCode::Success) {
            return err;
        }
        *value = (iface->ibuf[2] << 12) + (iface->ibuf[3] << 8) + (iface->ibuf[4] << 4) + iface->ibuf[5];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getMemory(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* channel) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_MEMORY);

        if (const ErrorCode err = sendPacketWithReply(iface, camera, &packet); err != ErrorCode::Success) {
            return err;
        }
        *channel = iface->ibuf[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getDisplay(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DISPLAY);

        if (const ErrorCode err = sendPacketWithReply(iface, camera, &packet); err != ErrorCode::Success) {
            return err;
        }
        *power = iface->ibuf[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getId(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* id) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ID);

        if (const ErrorCode err = sendPacketWithReply(iface, camera, &packet); err != ErrorCode::Success) {
            return err;
        }
        *id = (iface->ibuf[2] << 12) + (iface->ibuf[3] << 8) + (iface->ibuf[4] << 4) + iface->ibuf[5];
        return ErrorCode::Success;
    }

    ErrorCode Visca::setIrreceiveOn(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_IRRECEIVE);
        appendByte(&packet, VISCA_IRRECEIVE_ON);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setIrreceiveOff(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_IRRECEIVE);
        appendByte(&packet, VISCA_IRRECEIVE_OFF);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setIrreceiveOnoff(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_IRRECEIVE);
        appendByte(&packet, VISCA_IRRECEIVE_ONOFF);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setPantiltUp(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t pan_speed,
                                   const uint32_t tilt_speed) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_DRIVE);
        appendByte(&packet, pan_speed);
        appendByte(&packet, tilt_speed);
        appendByte(&packet, VISCA_PT_DRIVE_HORIZ_STOP);
        appendByte(&packet, VISCA_PT_DRIVE_VERT_UP);
        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setPantiltDown(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t pan_speed,
                                     const uint32_t tilt_speed) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_DRIVE);
        appendByte(&packet, pan_speed);
        appendByte(&packet, tilt_speed);
        appendByte(&packet, VISCA_PT_DRIVE_HORIZ_STOP);
        appendByte(&packet, VISCA_PT_DRIVE_VERT_DOWN);
        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setPantiltLeft(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t pan_speed,
                                     const uint32_t tilt_speed) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_DRIVE);
        appendByte(&packet, pan_speed);
        appendByte(&packet, tilt_speed);
        appendByte(&packet, VISCA_PT_DRIVE_HORIZ_LEFT);
        appendByte(&packet, VISCA_PT_DRIVE_VERT_STOP);
        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setPantiltRight(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t pan_speed,
                                      const uint32_t tilt_speed) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_DRIVE);
        appendByte(&packet, pan_speed);
        appendByte(&packet, tilt_speed);
        appendByte(&packet, VISCA_PT_DRIVE_HORIZ_RIGHT);
        appendByte(&packet, VISCA_PT_DRIVE_VERT_STOP);
        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setPantiltUpleft(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t pan_speed,
                                       const uint32_t tilt_speed) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_DRIVE);
        appendByte(&packet, pan_speed);
        appendByte(&packet, tilt_speed);
        appendByte(&packet, VISCA_PT_DRIVE_HORIZ_LEFT);
        appendByte(&packet, VISCA_PT_DRIVE_VERT_UP);
        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setPantiltUpright(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t pan_speed,
                                        const uint32_t tilt_speed) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_DRIVE);
        appendByte(&packet, pan_speed);
        appendByte(&packet, tilt_speed);
        appendByte(&packet, VISCA_PT_DRIVE_HORIZ_RIGHT);
        appendByte(&packet, VISCA_PT_DRIVE_VERT_UP);
        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setPantiltDownleft(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t pan_speed,
                                         const uint32_t tilt_speed) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_DRIVE);
        appendByte(&packet, pan_speed);
        appendByte(&packet, tilt_speed);
        appendByte(&packet, VISCA_PT_DRIVE_HORIZ_LEFT);
        appendByte(&packet, VISCA_PT_DRIVE_VERT_DOWN);
        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setPantiltDownright(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t pan_speed,
                                          const uint32_t tilt_speed) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_DRIVE);
        appendByte(&packet, pan_speed);
        appendByte(&packet, tilt_speed);
        appendByte(&packet, VISCA_PT_DRIVE_HORIZ_RIGHT);
        appendByte(&packet, VISCA_PT_DRIVE_VERT_DOWN);
        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setPantiltStop(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t pan_speed,
                                     const uint32_t tilt_speed) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_DRIVE);
        appendByte(&packet, pan_speed);
        appendByte(&packet, tilt_speed);
        appendByte(&packet, VISCA_PT_DRIVE_HORIZ_STOP);
        appendByte(&packet, VISCA_PT_DRIVE_VERT_STOP);
        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setPantiltAbsolutePosition(ViscaInterface* iface, const ViscaCamera* camera,
                                                 const uint32_t pan_speed, const uint32_t tilt_speed,
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

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setPantiltRelativePosition(ViscaInterface* iface, const ViscaCamera* camera,
                                                 const uint32_t pan_speed, const uint32_t tilt_speed,
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

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setPantiltHome(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_HOME);
        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setPantiltReset(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_RESET);
        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setPantiltLimitUpright(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t pan_pos,
                                             const uint32_t tilt_pos) {
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

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setPantiltLimitDownleft(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t pan_pos,
                                              const uint32_t tilt_pos) {
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

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setPantiltLimitDownleftClear(ViscaInterface* iface, const ViscaCamera* camera) {
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

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setPantiltLimitUprightClear(ViscaInterface* iface, const ViscaCamera* camera) {
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

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setDatascreenOn(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_DATASCREEN);
        appendByte(&packet, VISCA_PT_DATASCREEN_ON);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setDatascreenOff(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_DATASCREEN);
        appendByte(&packet, VISCA_PT_DATASCREEN_OFF);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setDatascreenOnoff(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_DATASCREEN);
        appendByte(&packet, VISCA_PT_DATASCREEN_ONOFF);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setRegister(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t reg_num,
                                  const uint8_t reg_val) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_REGISTER_VALUE);
        appendByte(&packet, reg_num);
        appendByte(&packet, (reg_val & 0xF0) >> 4);
        appendByte(&packet, (reg_val & 0x0F));
        return sendPacketWithReply(iface, camera, &packet);
    }

    /***********************************/
    /*       INQUIRY FUNCTIONS         */
    /***********************************/

    ErrorCode Visca::getVideosystem(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* system) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_VIDEOSYSTEM_INQ);

        if (const ErrorCode err = sendPacketWithReply(iface, camera, &packet); err != ErrorCode::Success) {
            return err;
        }
        *system = iface->ibuf[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getPantiltMode(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* status) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_MODE_INQ);

        if (const ErrorCode err = sendPacketWithReply(iface, camera, &packet); err != ErrorCode::Success) {
            return err;
        }
        *status = ((iface->ibuf[2] & 0xff) << 8) + (iface->ibuf[3] & 0xff);
        return ErrorCode::Success;
    }

    ErrorCode Visca::getPantiltMaxspeed(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* max_pan_speed,
                                         uint8_t* max_tilt_speed) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_MAXSPEED_INQ);

        if (const ErrorCode err = sendPacketWithReply(iface, camera, &packet); err != ErrorCode::Success) {
            return err;
        }
        *max_pan_speed = (iface->ibuf[2] & 0xff);
        *max_tilt_speed = (iface->ibuf[3] & 0xff);
        return ErrorCode::Success;
    }

    ErrorCode Visca::getPantiltPosition(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* pan_position,
                                         uint16_t* tilt_position) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_POSITION_INQ);

        if (const ErrorCode err = sendPacketWithReply(iface, camera, &packet); err != ErrorCode::Success) {
            return err;
        }
        *pan_position = ((iface->ibuf[2] & 0xf) << 12) + ((iface->ibuf[3] & 0xf) << 8) + ((iface->ibuf[4] & 0xf) << 4) +
            (iface->ibuf[5] & 0xf);
        *tilt_position = ((iface->ibuf[6] & 0xf) << 12) + ((iface->ibuf[7] & 0xf) << 8) + ((iface->ibuf[8] & 0xf) << 4)
            + (iface->ibuf[9] & 0xf);

        return ErrorCode::Success;
    }

    ErrorCode Visca::getDatascreen(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* status) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_DATASCREEN_INQ);

        if (const ErrorCode err = sendPacketWithReply(iface, camera, &packet); err != ErrorCode::Success) {
            return err;
        }
        *status = iface->ibuf[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getRegister(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t reg_num,
                                  uint8_t* reg_val) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_REGISTER_VALUE);
        appendByte(&packet, reg_num);

        if (const ErrorCode err = sendPacketWithReply(iface, camera, &packet); err != ErrorCode::Success) {
            return err;
        }
        *reg_val = (iface->ibuf[2] << 4) + iface->ibuf[3];
        return ErrorCode::Success;
    }

    /********************************/
    /* SPECIAL FUNCTIONS FOR D30/31 */
    /********************************/

    ErrorCode Visca::setWideConLens(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_WIDE_CON_LENS);
        appendByte(&packet, VISCA_WIDE_CON_LENS_SET);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setAtModeOnoff(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_MODE);
        appendByte(&packet, VISCA_AT_ONOFF);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setAtMode(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_MODE);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setAtAeOnoff(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_AE);
        appendByte(&packet, VISCA_AT_ONOFF);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setAtAe(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_AE);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setAtAutozoomOnoff(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_AUTOZOOM);
        appendByte(&packet, VISCA_AT_ONOFF);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setAtAutozoom(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_AUTOZOOM);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setAtmdFramedisplayOnoff(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_ATMD_FRAMEDISPLAY);
        appendByte(&packet, VISCA_AT_ONOFF);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setAtmdFramedisplay(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_ATMD_FRAMEDISPLAY);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setAtFrameoffsetOnoff(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_FRAMEOFFSET);
        appendByte(&packet, VISCA_AT_ONOFF);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setAtFrameoffset(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_FRAMEOFFSET);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setAtmdStartstop(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_ATMD_STARTSTOP);
        appendByte(&packet, VISCA_AT_ONOFF);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setAtChase(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_CHASE);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setAtChaseNext(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_CHASE);
        appendByte(&packet, VISCA_AT_CHASE_NEXT);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setMdModeOnoff(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_MODE);
        appendByte(&packet, VISCA_MD_ONOFF);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setMdMode(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_MODE);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setMdFrame(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_FRAME);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setMdDetect(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_DETECT);
        appendByte(&packet, VISCA_MD_ONOFF);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setAtEntry(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_ENTRY);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setAtLostinfo(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_ATMD_LOSTINFO1);
        appendByte(&packet, VISCA_ATMD_LOSTINFO2);
        appendByte(&packet, VISCA_AT_LOSTINFO);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setMdLostinfo(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_ATMD_LOSTINFO1);
        appendByte(&packet, VISCA_ATMD_LOSTINFO2);
        appendByte(&packet, VISCA_MD_LOSTINFO);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setMdAdjustYlevel(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_YLEVEL);
        appendByte(&packet, VISCA_MD_ADJUST);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setMdAdjustHuelevel(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_HUELEVEL);
        appendByte(&packet, VISCA_MD_ADJUST);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setMdAdjustSize(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_SIZE);
        appendByte(&packet, VISCA_MD_ADJUST);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setMdAdjustDisptime(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_DISPTIME);
        appendByte(&packet, VISCA_MD_ADJUST);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setMdAdjustRefmode(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_REFMODE);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setMdAdjustReftime(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_REFTIME);
        appendByte(&packet, VISCA_MD_ADJUST);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setMdMeasureMode1Onoff(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_MEASURE_MODE_1);
        appendByte(&packet, VISCA_MD_ONOFF);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setMdMeasureMode1(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_MEASURE_MODE_1);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setMdMeasureMode2Onoff(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_MEASURE_MODE_2);
        appendByte(&packet, VISCA_MD_ONOFF);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::setMdMeasureMode2(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_MEASURE_MODE_2);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    ErrorCode Visca::getKeylock(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_KEYLOCK);

        if (const ErrorCode err = sendPacketWithReply(iface, camera, &packet); err != ErrorCode::Success) {
            return err;
        }
        *power = iface->ibuf[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getWideConLens(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_WIDE_CON_LENS);

        if (const ErrorCode err = sendPacketWithReply(iface, camera, &packet); err != ErrorCode::Success) {
            return err;
        }
        *power = iface->ibuf[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getAtmdMode(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_ATMD_MODE);

        if (const ErrorCode err = sendPacketWithReply(iface, camera, &packet); err != ErrorCode::Success) {
            return err;
        }
        *power = iface->ibuf[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getAtMode(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_MODE_QUERY);

        if (const ErrorCode err = sendPacketWithReply(iface, camera, &packet); err != ErrorCode::Success) {
            return err;
        }
        *value = ((iface->ibuf[2] & 0xff) << 8) + (iface->ibuf[3] & 0xff);
        return ErrorCode::Success;
    }

    ErrorCode Visca::getAtEntry(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_ENTRY);

        if (const ErrorCode err = sendPacketWithReply(iface, camera, &packet); err != ErrorCode::Success) {
            return err;
        }
        *power = iface->ibuf[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getMdMode(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_MODE_QUERY);
        if (const ErrorCode err = sendPacketWithReply(iface, camera, &packet); err != ErrorCode::Success) {
            return err;
        }
        *value = ((iface->ibuf[2] & 0xff) << 8) + (iface->ibuf[3] & 0xff);
        return ErrorCode::Success;
    }

    ErrorCode Visca::getMdYlevel(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_YLEVEL);
        if (const ErrorCode err = sendPacketWithReply(iface, camera, &packet); err != ErrorCode::Success) {
            return err;
        }
        *power = (iface->ibuf[3] & 0x0f);
        return ErrorCode::Success;
    }

    ErrorCode Visca::getMdHuelevel(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_HUELEVEL);
        if (const ErrorCode err = sendPacketWithReply(iface, camera, &packet); err != ErrorCode::Success) {
            return err;
        }
        *power = (iface->ibuf[3] & 0x0f);
        return ErrorCode::Success;
    }

    ErrorCode Visca::getMdSize(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_SIZE);
        if (const ErrorCode err = sendPacketWithReply(iface, camera, &packet); err != ErrorCode::Success) {
            return err;
        }
        *power = (iface->ibuf[3] & 0x0f);
        return ErrorCode::Success;
    }

    ErrorCode Visca::getMdDisptime(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_DISPTIME);

        if (const ErrorCode err = sendPacketWithReply(iface, camera, &packet); err != ErrorCode::Success) {
            return err;
        }
        *power = (iface->ibuf[3] & 0x0f);
        return ErrorCode::Success;
    }

    ErrorCode Visca::getMdRefmode(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_REFMODE);

        if (const ErrorCode err = sendPacketWithReply(iface, camera, &packet); err != ErrorCode::Success) {
            return err;
        }
        *power = iface->ibuf[2];
        return ErrorCode::Success;
    }

    ErrorCode Visca::getMdReftime(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_REFTIME_QUERY);

        if (const ErrorCode err = sendPacketWithReply(iface, camera, &packet); err != ErrorCode::Success) {
            return err;
        }
        *power = (iface->ibuf[3] & 0x0f);
        return ErrorCode::Success;
    }

    ErrorCode Visca::getAtObjPos(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* xpos, uint8_t* ypos,
                                  uint8_t* status) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_POSITION);

        if (const ErrorCode err = sendPacketWithReply(iface, camera, &packet); err != ErrorCode::Success) {
            return err;
        }
        *xpos = iface->ibuf[2];
        *ypos = iface->ibuf[3];
        *status = (iface->ibuf[4] & 0x0f);
        return ErrorCode::Success;
    }

    ErrorCode Visca::getMdObjPos(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* xpos, uint8_t* ypos,
                                  uint8_t* status) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_POSITION);

        if (const ErrorCode err = sendPacketWithReply(iface, camera, &packet); err != ErrorCode::Success) {
            return err;
        }
        *xpos = iface->ibuf[2];
        *ypos = iface->ibuf[3];
        *status = (iface->ibuf[4] & 0x0f);
        return ErrorCode::Success;
    }
}