#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "libvisca.h"

void _VISCA_append_byte(VISCAPacket_t* packet, unsigned char byte);
void _VISCA_init_packet(VISCAPacket_t* packet);
unsigned int _VISCA_get_reply(VISCAInterface_t* iface, VISCACamera_t* camera);
unsigned int _VISCA_send_packet_with_reply(VISCAInterface_t* iface, VISCACamera_t* camera, VISCAPacket_t* packet);

uint32_t _VISCA_write_packet_data(const VISCAInterface_t* iface, VISCACamera_t* camera, const VISCAPacket_t* packet) {
    const int err = write(iface->port_fd, packet->bytes, packet->length);
    if (err < packet->length) {
        return VISCA_FAILURE;
    }
    return VISCA_SUCCESS;
}

uint32_t _VISCA_send_packet(const VISCAInterface_t* iface, VISCACamera_t* camera, VISCAPacket_t* packet) {
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
    _VISCA_append_byte(packet,VISCA_TERMINATOR);

    return _VISCA_write_packet_data(iface, camera, packet);
}

uint32_t _VISCA_get_packet(VISCAInterface_t* iface) {
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

uint32_t VISCA_open_serial(VISCAInterface_t* iface, const char* device_name) {
    const int fd = open(device_name, O_RDWR | O_NDELAY | O_NOCTTY);

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

uint32_t VISCA_unread_bytes(const VISCAInterface_t* iface, unsigned char* buffer, uint32_t* buffer_size) {
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

uint32_t VISCA_close_serial(VISCAInterface_t* iface) {
    if (iface->port_fd != -1) {
        close(iface->port_fd);
        iface->port_fd = -1;
        return VISCA_SUCCESS;
    }
    return VISCA_FAILURE;
}