#include "UartTransport.h"

#include <fcntl.h>
#include <stdexcept>
#include <unistd.h>
#include <sys/ioctl.h>

namespace camera_service::infrastructure {
    UartTransport::UartTransport(const std::string& device_path) : device_path_(device_path) {
        if (device_path_.empty()) {
            throw std::invalid_argument("Device path cannot be empty");
        }
    }

    UartTransport::~UartTransport() {
        disconnect();
    }

    bool UartTransport::connect() {
        const auto fd = open(device_path_.c_str(), O_RDWR | O_NDELAY | O_NOCTTY);

        if (fd == -1) {
            port_fd_ = -1;
            return false;
        }
        fcntl(fd, F_SETFL, 0);
        /* Setting port parameters */
        tcgetattr(fd, &options_);

        /* control flags */
        cfsetispeed(&options_,B9600); /* 9600 Bds   */
        options_.c_cflag &= ~PARENB; /* No parity  */
        options_.c_cflag &= ~CSTOPB; /*            */
        options_.c_cflag &= ~CSIZE; /* 8bit       */
        options_.c_cflag |= CS8; /*            */
        options_.c_cflag &= ~CRTSCTS; /* No hdw ctl */

        /* local flags */
        options_.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); /* raw input */

        /* input flags */
        /*
            options.c_iflag &= ~(INPCK | ISTRIP); // no parity
            options.c_iflag &= ~(IXON | IXOFF | IXANY); // no soft ctl
            */
        /* patch: bpflegin: set to 0 in order to avoid invalid pan/tilt return values */
        options_.c_iflag = 0;

        /* output flags */
        options_.c_oflag &= ~OPOST; /* raw output */

        tcsetattr(fd, TCSANOW, &options_);
        port_fd_ = fd;
        address_ = 0;

        return true;
    }

    void UartTransport::disconnect() {
        if (port_fd_ != -1) {
            ::close(port_fd_);
            port_fd_ = -1;
        }
    }

    bool UartTransport::write(const std::span<uint8_t> buffer) const {
        if (const auto err = ::write(port_fd_, buffer.data(), buffer.size_bytes()); err < buffer.size_bytes()) {
            return false;
        }
        return true;
    }

    bool UartTransport::read() {
        int pos = 0;

        // wait for message
        ioctl(port_fd_, FIONREAD, &(bytes_));
        while (bytes_ == 0) {
            usleep(0);
            ioctl(port_fd_, FIONREAD, &(bytes_));
        }

        // get octets one by one
        ::read(port_fd_, ibuf_.data(), 1);
        while (ibuf_.at(pos) != 0xFF) { //FIXME: use VISCA_TERMINATOR instead of 0xFF
            pos++;
            ::read(port_fd_, &ibuf_.at(pos), 1);
            usleep(0);
        }
        bytes_ = pos + 1;

        return true;
    }
}

