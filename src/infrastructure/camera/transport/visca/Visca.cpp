#include "Visca.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

namespace camera_service::infrastructure {
    error_code Visca::writePacketData(const ViscaInterface* iface, const ViscaPacket* packet) {
        const auto err = write(iface->port_fd, packet->bytes, packet->length);
        if (err < packet->length) {
            return VISCA_FAILURE;
        }
        return VISCA_SUCCESS;
    }

    error_code Visca::sendPacket(const ViscaInterface* iface, const ViscaCamera* camera, ViscaPacket* packet) {
        // check data:
        if ((iface->address > 7) || (camera->address > 7) || (iface->broadcast > 1)) {
            return VISCA_FAILURE;
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
        appendByte(packet,VISCA_TERMINATOR);

        return writePacketData(iface, packet);
    }

    error_code Visca::getPacket(ViscaInterface* iface) {
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

        return VISCA_SUCCESS;
    }

    /***********************************/
    /*       SYSTEM  FUNCTIONS         */
    /***********************************/

    error_code Visca::openSerial(ViscaInterface* iface, const char* device_name) {
        const auto fd = open(device_name, O_RDWR | O_NDELAY | O_NOCTTY);

        if (fd == -1) {
            iface->port_fd = -1;
            return VISCA_FAILURE;
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

        return VISCA_SUCCESS;
    }

    error_code Visca::closeSerial(ViscaInterface* iface) {
        if (iface->port_fd != -1) {
            close(iface->port_fd);
            iface->port_fd = -1;
            return VISCA_SUCCESS;
        }
        return VISCA_FAILURE;
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

    error_code Visca::getReply(ViscaInterface* iface) {
        // first message: -------------------
        if (getPacket(iface) != VISCA_SUCCESS) {
            return VISCA_FAILURE;
        }
        iface->type = iface->ibuf[1] & 0xF0;

        // skip ack messages
        while (iface->type == VISCA_RESPONSE_ACK) {
            if (getPacket(iface) != VISCA_SUCCESS) {
                return VISCA_FAILURE;
            }
            iface->type = iface->ibuf[1] & 0xF0;
        }

        switch (iface->type) {
        case VISCA_RESPONSE_CLEAR:
        case VISCA_RESPONSE_ADDRESS:
        case VISCA_RESPONSE_COMPLETED:
        case VISCA_RESPONSE_ERROR:
            return VISCA_SUCCESS;
            break;
        default:
            return VISCA_FAILURE;
        }
    }

    error_code Visca::sendPacketWithReply(ViscaInterface* iface, const ViscaCamera* camera, ViscaPacket* packet) {
        if (sendPacket(iface, camera, packet) != VISCA_SUCCESS) {
            return VISCA_FAILURE;
        }

        if (getReply(iface) != VISCA_SUCCESS) {
            return VISCA_FAILURE;
        }

        return VISCA_SUCCESS;
    }

    error_code Visca::unreadBytes(const ViscaInterface* iface, unsigned char* buffer, uint32_t* buffer_size) {
        uint32_t bytes = 0;
        *buffer_size = 0;

        ioctl(iface->port_fd, FIONREAD, &bytes);
        if (bytes > 0) {
            bytes = (bytes > *buffer_size) ? *buffer_size : bytes;
            read(iface->port_fd, &buffer, bytes);
            *buffer_size = bytes;
            return VISCA_FAILURE;
        }
        return VISCA_SUCCESS;
    }

    /****************************************************************************/
    /*                           PUBLIC FUNCTIONS                               */
    /****************************************************************************/

    /***********************************/
    /*       SYSTEM  FUNCTIONS         */
    /***********************************/

    error_code Visca::setAddress(ViscaInterface* iface, int* camera_num) {
        ViscaPacket packet{};
        ViscaCamera camera{};

        camera.address = 0;
        const auto backup = iface->broadcast;

        initPacket(&packet);
        appendByte(&packet, 0x30);
        appendByte(&packet, 0x01);

        iface->broadcast = 1;
        if (sendPacket(iface, &camera, &packet) != VISCA_SUCCESS) {
            iface->broadcast = backup;
            return VISCA_FAILURE;
        }
        iface->broadcast = backup;

        if (getReply(iface) != VISCA_SUCCESS) {
            return VISCA_FAILURE;
        }
        /* We parse the message from the camera here  */
        /* We expect to receive 4*camera_num bytes,
               every packet should be 88 30 0x FF, x being
               the camera id+1. The number of cams will thus be
               ibuf[bytes-2]-1  */
        if ((iface->bytes & 0x3) != 0) {
            /* check multiple of 4 */
            return VISCA_FAILURE;
        }
        *camera_num = iface->ibuf[iface->bytes - 2] - 1;
        if ((*camera_num == 0) || (*camera_num > 7)) {
            return VISCA_FAILURE;
        }
        return VISCA_SUCCESS;
    }

    error_code Visca::clear(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, 0x01);
        appendByte(&packet, 0x00);
        appendByte(&packet, 0x01);

        if (sendPacket(iface, camera, &packet) != VISCA_SUCCESS) {
            return VISCA_FAILURE;
        }
        if (getReply(iface) != VISCA_SUCCESS) {
            return VISCA_FAILURE;
        }
        return VISCA_SUCCESS;
    }

    error_code Visca::getCameraInfo(ViscaInterface* iface, ViscaCamera* camera) {
        ViscaPacket packet{};
        packet.bytes[0] = 0x80 | camera->address;
        packet.bytes[1] = 0x09;
        packet.bytes[2] = 0x00;
        packet.bytes[3] = 0x02;
        packet.bytes[4] = VISCA_TERMINATOR;
        packet.length = 5;

        if (writePacketData(iface, &packet) != VISCA_SUCCESS) {
            return VISCA_FAILURE;
        }
        if (getReply(iface) != VISCA_SUCCESS) {
            return VISCA_FAILURE;
        }

        if (iface->bytes != 10) {
            /* we expect 10 bytes as answer */
            return VISCA_FAILURE;
        }
        camera->vendor = (iface->ibuf[2] << 8) + iface->ibuf[3];
        camera->model = (iface->ibuf[4] << 8) + iface->ibuf[5];
        camera->rom_version = (iface->ibuf[6] << 8) + iface->ibuf[7];
        camera->socket_num = iface->ibuf[8];
        return VISCA_SUCCESS;
    }

    /***********************************/
    /*       COMMAND FUNCTIONS         */
    /***********************************/

    error_code Visca::setPower(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_POWER);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setKeylock(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_KEYLOCK);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setCameraId(ViscaInterface* iface, const ViscaCamera* camera, const uint16_t id) {
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

    error_code Visca::setZoomTele(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ZOOM);
        appendByte(&packet, VISCA_ZOOM_TELE);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setZoomWide(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ZOOM);
        appendByte(&packet, VISCA_ZOOM_WIDE);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setZoomStop(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ZOOM);
        appendByte(&packet, VISCA_ZOOM_STOP);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setZoomTeleSpeed(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t speed) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ZOOM);
        appendByte(&packet, VISCA_ZOOM_TELE_SPEED | (speed & 0x7));

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setZoomWideSpeed(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t speed) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ZOOM);
        appendByte(&packet, VISCA_ZOOM_WIDE_SPEED | (speed & 0x7));

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setZoomValue(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t zoom) {
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

    error_code Visca::setZoomAndFocusValue(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t zoom,
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

    error_code Visca::setDzoom(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DZOOM);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setDzoomLimit(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t limit) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DZOOM_LIMIT);
        appendByte(&packet, limit);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setDzoomMode(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DZOOM_MODE);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setFocusFar(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS);
        appendByte(&packet, VISCA_FOCUS_FAR);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setFocusNear(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS);
        appendByte(&packet, VISCA_FOCUS_NEAR);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setFocusStop(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS);
        appendByte(&packet, VISCA_FOCUS_STOP);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setFocusFarSpeed(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t speed) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS);
        appendByte(&packet, VISCA_FOCUS_FAR_SPEED | (speed & 0x7));

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setFocusNearSpeed(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t speed) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS);
        appendByte(&packet, VISCA_FOCUS_NEAR_SPEED | (speed & 0x7));

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setFocusValue(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t focus) {
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

    error_code Visca::setFocusAuto(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_AUTO);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setFocusOnePush(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_ONE_PUSH);
        appendByte(&packet, VISCA_FOCUS_ONE_PUSH_TRIG);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setFocusInfinity(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_ONE_PUSH);
        appendByte(&packet, VISCA_FOCUS_ONE_PUSH_INF);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setFocusAutosenseHigh(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_AUTO_SENSE);
        appendByte(&packet, VISCA_FOCUS_AUTO_SENSE_HIGH);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setFocusAutosenseLow(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_AUTO_SENSE);
        appendByte(&packet, VISCA_FOCUS_AUTO_SENSE_LOW);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setFocusNearLimit(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t limit) {
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

    error_code Visca::setWhitebalMode(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t mode) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_WB);
        appendByte(&packet, mode);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setWhitebalOnePush(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_WB_TRIGGER);
        appendByte(&packet, VISCA_WB_ONE_PUSH_TRIG);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setRgainUp(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_RGAIN);
        appendByte(&packet, VISCA_UP);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setRgainDown(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_RGAIN);
        appendByte(&packet, VISCA_DOWN);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setRgainReset(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_RGAIN);
        appendByte(&packet, VISCA_RESET);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setRgainValue(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t value) {
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

    error_code Visca::setBgainUp(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BGAIN);
        appendByte(&packet, VISCA_UP);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setBgainDown(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BGAIN);
        appendByte(&packet, VISCA_DOWN);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setBgainReset(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BGAIN);
        appendByte(&packet, VISCA_RESET);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setBgainValue(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t value) {
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

    error_code Visca::setShutterUp(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_SHUTTER);
        appendByte(&packet, VISCA_UP);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setShutterDown(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_SHUTTER);
        appendByte(&packet, VISCA_DOWN);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setShutterReset(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_SHUTTER);
        appendByte(&packet, VISCA_RESET);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setShutterValue(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t value) {
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

    error_code Visca::setIrisUp(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_IRIS);
        appendByte(&packet, VISCA_UP);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setIrisDown(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_IRIS);
        appendByte(&packet, VISCA_DOWN);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setIrisReset(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_IRIS);
        appendByte(&packet, VISCA_RESET);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setIrisValue(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t value) {
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

    error_code Visca::setGainUp(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_GAIN);
        appendByte(&packet, VISCA_UP);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setGainDown(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_GAIN);
        appendByte(&packet, VISCA_DOWN);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setGainReset(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_GAIN);
        appendByte(&packet, VISCA_RESET);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setGainValue(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t value) {
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

    error_code Visca::setBrightUp(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BRIGHT);
        appendByte(&packet, VISCA_UP);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setBrightDown(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BRIGHT);
        appendByte(&packet, VISCA_DOWN);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setBrightReset(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BRIGHT);
        appendByte(&packet, VISCA_RESET);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setBrightValue(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t value) {
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

    error_code Visca::setApertureUp(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_APERTURE);
        appendByte(&packet, VISCA_UP);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setApertureDown(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_APERTURE);
        appendByte(&packet, VISCA_DOWN);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setApertureReset(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_APERTURE);
        appendByte(&packet, VISCA_RESET);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setApertureValue(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t value) {
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

    error_code Visca::setExpCompUp(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_EXP_COMP);
        appendByte(&packet, VISCA_UP);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setExpCompDown(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_EXP_COMP);
        appendByte(&packet, VISCA_DOWN);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setExpCompReset(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_EXP_COMP);
        appendByte(&packet, VISCA_RESET);

        return sendPacketWithReply(iface, camera, &packet);

        return VISCA_SUCCESS;
    }

    error_code Visca::setExpCompValue(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t value) {
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

    error_code Visca::setExpCompPower(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_EXP_COMP_POWER);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setAutoExpMode(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t mode) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_AUTO_EXP);
        appendByte(&packet, mode);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setSlowShutterAuto(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_SLOW_SHUTTER);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setBacklightComp(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BACKLIGHT_COMP);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setZeroLuxShot(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ZERO_LUX);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setIrLed(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_IR_LED);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setWideMode(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t mode) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_WIDE_MODE);
        appendByte(&packet, mode);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setMirror(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_MIRROR);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setFreeze(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FREEZE);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setPictureEffect(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t mode) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_PICTURE_EFFECT);
        appendByte(&packet, mode);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setDigitalEffect(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t mode) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DIGITAL_EFFECT);
        appendByte(&packet, mode);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setDigitalEffectLevel(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t level) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DIGITAL_EFFECT_LEVEL);
        appendByte(&packet, level);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setCamStabilizer(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_CAM_STABILIZER);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::memorySet(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t channel) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_MEMORY);
        appendByte(&packet, VISCA_MEMORY_SET);
        appendByte(&packet, channel);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::memoryRecall(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t channel) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_MEMORY);
        appendByte(&packet, VISCA_MEMORY_RECALL);
        appendByte(&packet, channel);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::memoryReset(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t channel) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_MEMORY);
        appendByte(&packet, VISCA_MEMORY_RESET);
        appendByte(&packet, channel);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setDisplay(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DISPLAY);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setDateTime(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t year,
                                const uint32_t month, const uint32_t day, const uint32_t hour, const uint32_t minute) {
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

    error_code Visca::setDateDisplay(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DATE_DISPLAY);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setTimeDisplay(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_TIME_DISPLAY);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setTitleDisplay(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_TITLE_DISPLAY);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setTitleClear(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_TITLE_DISPLAY);
        appendByte(&packet, VISCA_TITLE_DISPLAY_CLEAR);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setTitleParams(ViscaInterface* iface, const ViscaCamera* camera, const ViscaTitleData* title) {
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

    error_code Visca::setTitle(ViscaInterface* iface, const ViscaCamera* camera, const ViscaTitleData* title) {
        ViscaPacket packet{};
        error_code err = 0;

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_TITLE_SET);
        appendByte(&packet, VISCA_TITLE_SET_PART1);

        for (auto i = 0; i < 10; i++) {
            appendByte(&packet, title->title[i]);
        }

        err += sendPacketWithReply(iface, camera, &packet);

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_TITLE_SET);
        appendByte(&packet, VISCA_TITLE_SET_PART2);

        for (auto i = 0; i < 10; i++) {
            appendByte(&packet, title->title[i + 10]);
        }

        err += sendPacketWithReply(iface, camera, &packet);

        return err;
    }

    error_code Visca::setSpotAeOn(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_SPOT_AE);
        appendByte(&packet, VISCA_SPOT_AE_ON);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setSpotAeOff(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_SPOT_AE);
        appendByte(&packet, VISCA_SPOT_AE_OFF);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setSpotAePosition(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t x_position,
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

    error_code Visca::getPower(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_POWER);
        const error_code err = sendPacketWithReply(iface, camera, &packet);
        if (err != VISCA_SUCCESS) {
            return err;
        }
        *power = iface->ibuf[2];
        return VISCA_SUCCESS;
    }

    error_code Visca::getDzoom(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DZOOM);
        const error_code err = sendPacketWithReply(iface, camera, &packet);
        if (err != VISCA_SUCCESS) {
            return err;
        }
        *power = iface->ibuf[2];
        return VISCA_SUCCESS;
    }

    error_code Visca::getDzoomLimit(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DZOOM_LIMIT);
        const error_code err = sendPacketWithReply(iface, camera, &packet);
        if (err != VISCA_SUCCESS) {
            return err;
        }
        *value = iface->ibuf[2];
        return VISCA_SUCCESS;
    }

    error_code Visca::getZoomValue(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ZOOM_VALUE);
        const error_code err = sendPacketWithReply(iface, camera, &packet);
        if (err != VISCA_SUCCESS) {
            return err;
        }
        *value = (iface->ibuf[2] << 12) + (iface->ibuf[3] << 8) + (iface->ibuf[4] << 4) + iface->ibuf[5];
        return VISCA_SUCCESS;
    }

    error_code Visca::getFocusAuto(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_AUTO);
        const error_code err = sendPacketWithReply(iface, camera, &packet);
        if (err != VISCA_SUCCESS) {
            return err;
        }
        *power = iface->ibuf[2];
        return VISCA_SUCCESS;
    }

    error_code Visca::getFocusValue(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_VALUE);
        const error_code err = sendPacketWithReply(iface, camera, &packet);
        if (err != VISCA_SUCCESS) {
            return err;
        }
        *value = (iface->ibuf[2] << 12) + (iface->ibuf[3] << 8) + (iface->ibuf[4] << 4) + iface->ibuf[5];
        return VISCA_SUCCESS;
    }

    error_code Visca::getFocusAutoSense(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* mode) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_AUTO_SENSE);
        const error_code err = sendPacketWithReply(iface, camera, &packet);
        if (err != VISCA_SUCCESS) {
            return err;
        }
        *mode = iface->ibuf[2];
        return VISCA_SUCCESS;
    }

    error_code Visca::getFocusNearLimit(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FOCUS_NEAR_LIMIT);
        const error_code err = sendPacketWithReply(iface, camera, &packet);
        if (err != VISCA_SUCCESS) {
            return err;
        }
        *value = (iface->ibuf[2] << 12) + (iface->ibuf[3] << 8) + (iface->ibuf[4] << 4) + iface->ibuf[5];
        return VISCA_SUCCESS;
    }

    error_code Visca::getWhitebalMode(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* mode) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_WB);
        const error_code err = sendPacketWithReply(iface, camera, &packet);
        if (err != VISCA_SUCCESS) {
            return err;
        }
        *mode = iface->ibuf[2];
        return VISCA_SUCCESS;
    }

    error_code Visca::getRgainValue(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_RGAIN_VALUE);
        const error_code err = sendPacketWithReply(iface, camera, &packet);
        if (err != VISCA_SUCCESS) {
            return err;
        }
        *value = (iface->ibuf[2] << 12) + (iface->ibuf[3] << 8) + (iface->ibuf[4] << 4) + iface->ibuf[5];
        return VISCA_SUCCESS;
    }

    error_code Visca::getBgainValue(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BGAIN_VALUE);
        const error_code err = sendPacketWithReply(iface, camera, &packet);
        if (err != VISCA_SUCCESS) {
            return err;
        }
        *value = (iface->ibuf[2] << 12) + (iface->ibuf[3] << 8) + (iface->ibuf[4] << 4) + iface->ibuf[5];
        return VISCA_SUCCESS;
    }

    error_code Visca::getAutoExpMode(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* mode) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_AUTO_EXP);
        const error_code err = sendPacketWithReply(iface, camera, &packet);
        if (err != VISCA_SUCCESS) {
            return err;
        }
        *mode = iface->ibuf[2];
        return VISCA_SUCCESS;
    }

    error_code Visca::getSlowShutterAuto(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* mode) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_SLOW_SHUTTER);
        const error_code err = sendPacketWithReply(iface, camera, &packet);
        if (err != VISCA_SUCCESS) {
            return err;
        }
        *mode = iface->ibuf[2];
        return VISCA_SUCCESS;
    }

    error_code Visca::getShutterValue(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_SHUTTER_VALUE);
        const error_code err = sendPacketWithReply(iface, camera, &packet);
        if (err != VISCA_SUCCESS) {
            return err;
        }
        *value = (iface->ibuf[2] << 12) + (iface->ibuf[3] << 8) + (iface->ibuf[4] << 4) + iface->ibuf[5];
        return VISCA_SUCCESS;
    }

    error_code Visca::getIrisValue(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_IRIS_VALUE);
        const error_code err = sendPacketWithReply(iface, camera, &packet);
        if (err != VISCA_SUCCESS) {
            return err;
        }
        *value = (iface->ibuf[2] << 12) + (iface->ibuf[3] << 8) + (iface->ibuf[4] << 4) + iface->ibuf[5];
        return VISCA_SUCCESS;
    }

    error_code Visca::getGainValue(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_GAIN_VALUE);
        const error_code err = sendPacketWithReply(iface, camera, &packet);
        if (err != VISCA_SUCCESS) {
            return err;
        }
        *value = (iface->ibuf[2] << 12) + (iface->ibuf[3] << 8) + (iface->ibuf[4] << 4) + iface->ibuf[5];
        return VISCA_SUCCESS;
    }

    error_code Visca::getBrightValue(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BRIGHT_VALUE);
        const error_code err = sendPacketWithReply(iface, camera, &packet);
        if (err != VISCA_SUCCESS) {
            return err;
        }
        *value = (iface->ibuf[2] << 12) + (iface->ibuf[3] << 8) + (iface->ibuf[4] << 4) + iface->ibuf[5];
        return VISCA_SUCCESS;
    }

    error_code Visca::getExpCompPower(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_EXP_COMP_POWER);
        const error_code err = sendPacketWithReply(iface, camera, &packet);
        if (err != VISCA_SUCCESS) {
            return err;
        }
        *power = iface->ibuf[2];
        return VISCA_SUCCESS;
    }

    error_code Visca::getExpCompValue(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_EXP_COMP_VALUE);
        const error_code err = sendPacketWithReply(iface, camera, &packet);
        if (err != VISCA_SUCCESS) {
            return err;
        }
        *value = (iface->ibuf[2] << 12) + (iface->ibuf[3] << 8) + (iface->ibuf[4] << 4) + iface->ibuf[5];
        return VISCA_SUCCESS;
    }

    error_code Visca::getBacklightComp(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_BACKLIGHT_COMP);
        const error_code err = sendPacketWithReply(iface, camera, &packet);
        if (err != VISCA_SUCCESS) {
            return err;
        }
        *power = iface->ibuf[2];
        return VISCA_SUCCESS;
    }

    error_code Visca::getApertureValue(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_APERTURE_VALUE);
        const error_code err = sendPacketWithReply(iface, camera, &packet);
        if (err != VISCA_SUCCESS) {
            return err;
        }
        *value = (iface->ibuf[2] << 12) + (iface->ibuf[3] << 8) + (iface->ibuf[4] << 4) + iface->ibuf[5];
        return VISCA_SUCCESS;
    }

    error_code Visca::getZeroLuxShot(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ZERO_LUX);
        const error_code err = sendPacketWithReply(iface, camera, &packet);
        if (err != VISCA_SUCCESS) {
            return err;
        }
        *power = iface->ibuf[2];
        return VISCA_SUCCESS;
    }

    error_code Visca::getIrLed(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_IR_LED);
        const error_code err = sendPacketWithReply(iface, camera, &packet);
        if (err != VISCA_SUCCESS) {
            return err;
        }
        *power = iface->ibuf[2];
        return VISCA_SUCCESS;
    }

    error_code Visca::getWideMode(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* mode) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_WIDE_MODE);
        const error_code err = sendPacketWithReply(iface, camera, &packet);
        if (err != VISCA_SUCCESS) {
            return err;
        }
        *mode = iface->ibuf[2];
        return VISCA_SUCCESS;
    }

    error_code Visca::getMirror(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_MIRROR);
        const error_code err = sendPacketWithReply(iface, camera, &packet);
        if (err != VISCA_SUCCESS) {
            return err;
        }
        *power = iface->ibuf[2];
        return VISCA_SUCCESS;
    }

    error_code Visca::getFreeze(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_FREEZE);
        const error_code err = sendPacketWithReply(iface, camera, &packet);
        if (err != VISCA_SUCCESS) {
            return err;
        }
        *power = iface->ibuf[2];
        return VISCA_SUCCESS;
    }

    error_code Visca::getPictureEffect(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* mode) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_PICTURE_EFFECT);
        const error_code err = sendPacketWithReply(iface, camera, &packet);
        if (err != VISCA_SUCCESS) {
            return err;
        }
        *mode = iface->ibuf[2];
        return VISCA_SUCCESS;
    }

    error_code Visca::getDigitalEffect(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* mode) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DIGITAL_EFFECT);
        const error_code err = sendPacketWithReply(iface, camera, &packet);
        if (err != VISCA_SUCCESS) {
            return err;
        }
        *mode = iface->ibuf[2];
        return VISCA_SUCCESS;
    }

    error_code Visca::getDigitalEffectLevel(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DIGITAL_EFFECT_LEVEL);
        const error_code err = sendPacketWithReply(iface, camera, &packet);
        if (err != VISCA_SUCCESS) {
            return err;
        }
        *value = (iface->ibuf[2] << 12) + (iface->ibuf[3] << 8) + (iface->ibuf[4] << 4) + iface->ibuf[5];
        return VISCA_SUCCESS;
    }

    error_code Visca::getMemory(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* channel) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_MEMORY);
        const error_code err = sendPacketWithReply(iface, camera, &packet);
        if (err != VISCA_SUCCESS) {
            return err;
        }
        *channel = iface->ibuf[2];
        return VISCA_SUCCESS;
    }

    error_code Visca::getDisplay(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_DISPLAY);
        const error_code err = sendPacketWithReply(iface, camera, &packet);
        if (err != VISCA_SUCCESS) {
            return err;
        }
        *power = iface->ibuf[2];
        return VISCA_SUCCESS;
    }

    error_code Visca::getId(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* id) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_ID);
        const error_code err = sendPacketWithReply(iface, camera, &packet);
        if (err != VISCA_SUCCESS) {
            return err;
        }
        *id = (iface->ibuf[2] << 12) + (iface->ibuf[3] << 8) + (iface->ibuf[4] << 4) + iface->ibuf[5];
        return VISCA_SUCCESS;
    }

    error_code Visca::setIrreceiveOn(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_IRRECEIVE);
        appendByte(&packet, VISCA_IRRECEIVE_ON);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setIrreceiveOff(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_IRRECEIVE);
        appendByte(&packet, VISCA_IRRECEIVE_OFF);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setIrreceiveOnoff(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_IRRECEIVE);
        appendByte(&packet, VISCA_IRRECEIVE_ONOFF);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setPantiltUp(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t pan_speed,
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

    error_code Visca::setPantiltDown(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t pan_speed,
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

    error_code Visca::setPantiltLeft(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t pan_speed,
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

    error_code Visca::setPantiltRight(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t pan_speed,
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

    error_code Visca::setPantiltUpleft(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t pan_speed,
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

    error_code Visca::setPantiltUpright(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t pan_speed,
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

    error_code Visca::setPantiltDownleft(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t pan_speed,
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

    error_code Visca::setPantiltDownright(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t pan_speed,
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

    error_code Visca::setPantiltStop(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t pan_speed,
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

    error_code Visca::setPantiltAbsolutePosition(ViscaInterface* iface, const ViscaCamera* camera,
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

    error_code Visca::setPantiltRelativePosition(ViscaInterface* iface, const ViscaCamera* camera,
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

    error_code Visca::setPantiltHome(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_HOME);
        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setPantiltReset(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_RESET);
        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setPantiltLimitUpright(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t pan_pos,
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

    error_code Visca::setPantiltLimitDownleft(ViscaInterface* iface, const ViscaCamera* camera, const uint32_t pan_pos,
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

    error_code Visca::setPantiltLimitDownleftClear(ViscaInterface* iface, const ViscaCamera* camera) {
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

    error_code Visca::setPantiltLimitUprightClear(ViscaInterface* iface, const ViscaCamera* camera) {
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

    error_code Visca::setDatascreenOn(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_DATASCREEN);
        appendByte(&packet, VISCA_PT_DATASCREEN_ON);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setDatascreenOff(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_DATASCREEN);
        appendByte(&packet, VISCA_PT_DATASCREEN_OFF);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setDatascreenOnoff(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_DATASCREEN);
        appendByte(&packet, VISCA_PT_DATASCREEN_ONOFF);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setRegister(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t reg_num,
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

    error_code Visca::getVideosystem(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* system) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_VIDEOSYSTEM_INQ);
        const error_code err = sendPacketWithReply(iface, camera, &packet);
        if (err != VISCA_SUCCESS) {
            return err;
        }
        *system = iface->ibuf[2];
        return VISCA_SUCCESS;
    }

    error_code Visca::getPantiltMode(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* status) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_MODE_INQ);
        const error_code err = sendPacketWithReply(iface, camera, &packet);
        if (err != VISCA_SUCCESS) {
            return err;
        }
        *status = ((iface->ibuf[2] & 0xff) << 8) + (iface->ibuf[3] & 0xff);
        return VISCA_SUCCESS;
    }

    error_code Visca::getPantiltMaxspeed(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* max_pan_speed,
                                       uint8_t* max_tilt_speed) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_MAXSPEED_INQ);
        const error_code err = sendPacketWithReply(iface, camera, &packet);
        if (err != VISCA_SUCCESS) {
            return err;
        }
        *max_pan_speed = (iface->ibuf[2] & 0xff);
        *max_tilt_speed = (iface->ibuf[3] & 0xff);
        return VISCA_SUCCESS;
    }

    error_code Visca::getPantiltPosition(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* pan_position, uint16_t* tilt_position) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_POSITION_INQ);
        const error_code err = sendPacketWithReply(iface, camera, &packet);
        if (err != VISCA_SUCCESS) {
            return err;
        }
        *pan_position = ((iface->ibuf[2] & 0xf) << 12) + ((iface->ibuf[3] & 0xf) << 8) + ((iface->ibuf[4] & 0xf) << 4) +
            (iface->ibuf[5] & 0xf);
        *tilt_position = ((iface->ibuf[6] & 0xf) << 12) + ((iface->ibuf[7] & 0xf) << 8) + ((iface->ibuf[8] & 0xf) << 4)
            + (iface->ibuf[9] & 0xf);

        return VISCA_SUCCESS;
    }

    error_code Visca::getDatascreen(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* status) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_PT_DATASCREEN_INQ);
        const error_code err = sendPacketWithReply(iface, camera, &packet);
        if (err != VISCA_SUCCESS) {
            return err;
        }
        *status = iface->ibuf[2];
        return VISCA_SUCCESS;
    }

    error_code Visca::getRegister(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t reg_num,
                                uint8_t* reg_val) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_REGISTER_VALUE);
        appendByte(&packet, reg_num);
        const error_code err = sendPacketWithReply(iface, camera, &packet);
        if (err != VISCA_SUCCESS) {
            return err;
        }
        *reg_val = (iface->ibuf[2] << 4) + iface->ibuf[3];
        return VISCA_SUCCESS;
    }

    /********************************/
    /* SPECIAL FUNCTIONS FOR D30/31 */
    /********************************/

    error_code Visca::setWideConLens(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_WIDE_CON_LENS);
        appendByte(&packet, VISCA_WIDE_CON_LENS_SET);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setAtModeOnoff(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_MODE);
        appendByte(&packet, VISCA_AT_ONOFF);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setAtMode(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_MODE);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setAtAeOnoff(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_AE);
        appendByte(&packet, VISCA_AT_ONOFF);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setAtAe(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_AE);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setAtAutozoomOnoff(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_AUTOZOOM);
        appendByte(&packet, VISCA_AT_ONOFF);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setAtAutozoom(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_AUTOZOOM);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setAtmdFramedisplayOnoff(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_ATMD_FRAMEDISPLAY);
        appendByte(&packet, VISCA_AT_ONOFF);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setAtmdFramedisplay(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_ATMD_FRAMEDISPLAY);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setAtFrameoffsetOnoff(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_FRAMEOFFSET);
        appendByte(&packet, VISCA_AT_ONOFF);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setAtFrameoffset(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_FRAMEOFFSET);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setAtmdStartstop(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_ATMD_STARTSTOP);
        appendByte(&packet, VISCA_AT_ONOFF);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setAtChase(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_CHASE);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setAtChaseNext(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_CHASE);
        appendByte(&packet, VISCA_AT_CHASE_NEXT);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setMdModeOnoff(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_MODE);
        appendByte(&packet, VISCA_MD_ONOFF);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setMdMode(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_MODE);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setMdFrame(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_FRAME);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setMdDetect(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_DETECT);
        appendByte(&packet, VISCA_MD_ONOFF);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setAtEntry(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_ENTRY);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setAtLostinfo(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_ATMD_LOSTINFO1);
        appendByte(&packet, VISCA_ATMD_LOSTINFO2);
        appendByte(&packet, VISCA_AT_LOSTINFO);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setMdLostinfo(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&packet, VISCA_ATMD_LOSTINFO1);
        appendByte(&packet, VISCA_ATMD_LOSTINFO2);
        appendByte(&packet, VISCA_MD_LOSTINFO);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setMdAdjustYlevel(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_YLEVEL);
        appendByte(&packet, VISCA_MD_ADJUST);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setMdAdjustHuelevel(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_HUELEVEL);
        appendByte(&packet, VISCA_MD_ADJUST);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setMdAdjustSize(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_SIZE);
        appendByte(&packet, VISCA_MD_ADJUST);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setMdAdjustDisptime(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_DISPTIME);
        appendByte(&packet, VISCA_MD_ADJUST);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setMdAdjustRefmode(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_REFMODE);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setMdAdjustReftime(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_REFTIME);
        appendByte(&packet, VISCA_MD_ADJUST);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setMdMeasureMode1Onoff(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_MEASURE_MODE_1);
        appendByte(&packet, VISCA_MD_ONOFF);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setMdMeasureMode1(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_MEASURE_MODE_1);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setMdMeasureMode2Onoff(ViscaInterface* iface, const ViscaCamera* camera) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_MEASURE_MODE_2);
        appendByte(&packet, VISCA_MD_ONOFF);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::setMdMeasureMode2(ViscaInterface* iface, const ViscaCamera* camera, const uint8_t power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_COMMAND);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_MEASURE_MODE_2);
        appendByte(&packet, power);

        return sendPacketWithReply(iface, camera, &packet);
    }

    error_code Visca::getKeylock(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_KEYLOCK);
        const error_code err = sendPacketWithReply(iface, camera, &packet);
        if (err != VISCA_SUCCESS) {
            return err;
        }
        *power = iface->ibuf[2];
        return VISCA_SUCCESS;
    }

    error_code Visca::getWideConLens(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA1);
        appendByte(&packet, VISCA_WIDE_CON_LENS);
        const error_code err = sendPacketWithReply(iface, camera, &packet);
        if (err != VISCA_SUCCESS) {
            return err;
        }
        *power = iface->ibuf[2];
        return VISCA_SUCCESS;
    }

    error_code Visca::getAtmdMode(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_ATMD_MODE);
        const error_code err = sendPacketWithReply(iface, camera, &packet);
        if (err != VISCA_SUCCESS) {
            return err;
        }
        *power = iface->ibuf[2];
        return VISCA_SUCCESS;
    }

    error_code Visca::getAtMode(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_MODE_QUERY);
        const error_code err = sendPacketWithReply(iface, camera, &packet);
        if (err != VISCA_SUCCESS) {
            return err;
        }
        *value = ((iface->ibuf[2] & 0xff) << 8) + (iface->ibuf[3] & 0xff);
        return VISCA_SUCCESS;
    }

    error_code Visca::getAtEntry(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_ENTRY);
        const error_code err = sendPacketWithReply(iface, camera, &packet);
        if (err != VISCA_SUCCESS) {
            return err;
        }
        *power = iface->ibuf[2];
        return VISCA_SUCCESS;
    }

    error_code Visca::getMdMode(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_MODE_QUERY);
        const error_code err = sendPacketWithReply(iface, camera, &packet);
        if (err != VISCA_SUCCESS) {
            return err;
        }
        *value = ((iface->ibuf[2] & 0xff) << 8) + (iface->ibuf[3] & 0xff);
        return VISCA_SUCCESS;
    }

    error_code Visca::getMdYlevel(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_YLEVEL);
        const error_code err = sendPacketWithReply(iface, camera, &packet);
        if (err != VISCA_SUCCESS) {
            return err;
        }
        *power = (iface->ibuf[3] & 0x0f);
        return VISCA_SUCCESS;
    }

    error_code Visca::getMdHuelevel(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_HUELEVEL);
        const error_code err = sendPacketWithReply(iface, camera, &packet);
        if (err != VISCA_SUCCESS) {
            return err;
        }
        *power = (iface->ibuf[3] & 0x0f);
        return VISCA_SUCCESS;
    }

    error_code Visca::getMdSize(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_SIZE);
        const error_code err = sendPacketWithReply(iface, camera, &packet);
        if (err != VISCA_SUCCESS) {
            return err;
        }
        *power = (iface->ibuf[3] & 0x0f);
        return VISCA_SUCCESS;
    }

    error_code Visca::getMdDisptime(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_DISPTIME);
        const error_code err = sendPacketWithReply(iface, camera, &packet);
        if (err != VISCA_SUCCESS) {
            return err;
        }
        *power = (iface->ibuf[3] & 0x0f);
        return VISCA_SUCCESS;
    }

    error_code Visca::getMdRefmode(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_ADJUST_REFMODE);
        const error_code err = sendPacketWithReply(iface, camera, &packet);
        if (err != VISCA_SUCCESS) {
            return err;
        }
        *power = iface->ibuf[2];
        return VISCA_SUCCESS;
    }

    error_code Visca::getMdReftime(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_REFTIME_QUERY);
        const error_code err = sendPacketWithReply(iface, camera, &packet);
        if (err != VISCA_SUCCESS) {
            return err;
        }
        *power = (iface->ibuf[3] & 0x0f);
        return VISCA_SUCCESS;
    }

    error_code Visca::getAtObjPos(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* xpos, uint8_t* ypos,
                                uint8_t* status) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_AT_POSITION);
        const error_code err = sendPacketWithReply(iface, camera, &packet);
        if (err != VISCA_SUCCESS) {
            return err;
        }
        *xpos = iface->ibuf[2];
        *ypos = iface->ibuf[3];
        *status = (iface->ibuf[4] & 0x0f);
        return VISCA_SUCCESS;
    }

    error_code Visca::getMdObjPos(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* xpos, uint8_t* ypos,
                                uint8_t* status) {
        ViscaPacket packet{};

        initPacket(&packet);
        appendByte(&packet, VISCA_INQUIRY);
        appendByte(&packet, VISCA_CATEGORY_CAMERA2);
        appendByte(&packet, VISCA_MD_POSITION);
        const error_code err = sendPacketWithReply(iface, camera, &packet);
        if (err != VISCA_SUCCESS) {
            return err;
        }
        *xpos = iface->ibuf[2];
        *ypos = iface->ibuf[3];
        *status = (iface->ibuf[4] & 0x0f);
        return VISCA_SUCCESS;
    }
}