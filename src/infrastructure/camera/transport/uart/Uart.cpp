#include "Uart.h"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <utility>
#include <sys/select.h>

#include "common/Logger/Logger.h"

namespace camera_service::infrastructure {
    Uart::Uart(std::string device_path)
        : device_path_(std::move(device_path)) {
        // if (open().isError()) { //TODO: should we open here for RAII or in open()?
        //     LOG_ERROR("Failed to open UART device: {}", device_path_);
        // }
    }

    Uart::~Uart() {
        if (close().isError()) {
            LOG_ERROR("Failed to close UART device: {}", device_path_);
        }
    }

    Result<void> Uart::open() {
        if (isOpen()) {
            return Result<void>::error("UART device is already open");
        }

        const auto fd = ::open(device_path_.c_str(), O_RDWR | O_NDELAY | O_NOCTTY);
        if (fd < 0) {
            return Result<void>::error(
                "Failed to open UART device: " + device_path_ + " - " + std::string(strerror(errno)));
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
            options_.c_iflag &= ~(INPCK | ISTRIP); // no parity
            options_.c_iflag &= ~(IXON | IXOFF | IXANY); // no soft ctl
            */
        /* patch: bpflegin: set to 0 in order to avoid invalid pan/tilt return values */
        options_.c_iflag = 0;

        /* output flags */
        options_.c_oflag &= ~OPOST; /* raw output */

        tcsetattr(fd, TCSANOW, &options_);
        port_fd_ = fd;

        return Result<void>::success();
    }

    Result<void> Uart::close() {
        if (!isOpen()) {
            return Result<void>::success();
        }

        if (::close(port_fd_) < 0) {
            return Result<void>::error("Failed to close UART device: " + std::string(strerror(errno)));
        }

        port_fd_ = -1;
        return Result<void>::success();
    }

    Result<void> Uart::configure(const int baud_rate, const int data_bits, const int stop_bits,
                                 const char parity) const {
        if (!isOpen()) {
            return Result<void>::error("UART device is not open");
        }

        termios tty{};

        // Get current terminal settings
        if (tcgetattr(port_fd_, &tty) != 0) {
            return Result<void>::error("Failed to get terminal attributes: " + std::string(strerror(errno)));
        }

        // Set baud rate
        speed_t speed;
        switch (baud_rate) {
            case 9600:
                speed = B9600;
                break;
            case 19200:
                speed = B19200;
                break;
            case 38400:
                speed = B38400;
                break;
            case 57600:
                speed = B57600;
                break;
            case 115200:
                speed = B115200;
                break;
            case 230400:
                speed = B230400;
                break;
            case 460800:
                speed = B460800;
                break;
            case 500000:
                speed = B500000;
                break;
            case 576000:
                speed = B576000;
                break;
            case 921600:
                speed = B921600;
                break;
            case 1000000:
                speed = B1000000;
                break;
            case 1152000:
                speed = B1152000;
                break;
            case 1500000:
                speed = B1500000;
                break;
            case 2000000:
                speed = B2000000;
                break;
            case 2500000:
                speed = B2500000;
                break;
            case 3000000:
                speed = B3000000;
                break;
            case 3500000:
                speed = B3500000;
                break;
            case 4000000:
                speed = B4000000;
                break;
            default:
                return Result<void>::error("Unsupported baud rate: " + std::to_string(baud_rate));
        }

        cfsetispeed(&tty, speed);
        cfsetospeed(&tty, speed);

        // Configure data bits
        tty.c_cflag &= ~CSIZE; // Clear size bits
        switch (data_bits) {
            case 5:
                tty.c_cflag |= CS5;
                break;
            case 6:
                tty.c_cflag |= CS6;
                break;
            case 7:
                tty.c_cflag |= CS7;
                break;
            case 8:
                tty.c_cflag |= CS8;
                break;
            default:
                return Result<void>::error("Unsupported data bits: " + std::to_string(data_bits));
        }

        // Configure stop bits
        if (stop_bits == 1) {
            tty.c_cflag &= ~CSTOPB; // 1 stop bit
        } else if (stop_bits == 2) {
            tty.c_cflag |= CSTOPB; // 2 stop bits
        } else {
            return Result<void>::error("Unsupported stop bits: " + std::to_string(stop_bits));
        }

        // Configure parity
        switch (parity) {
            case 'N': // No parity
            case 'n':
                tty.c_cflag &= ~PARENB;
                break;
            case 'E': // Even parity
            case 'e':
                tty.c_cflag |= PARENB;
                tty.c_cflag &= ~PARODD;
                break;
            case 'O': // Odd parity
            case 'o':
                tty.c_cflag |= PARENB;
                tty.c_cflag |= PARODD;
                break;
            default:
                return Result<void>::error("Unsupported parity: " + std::string(1, parity));
        }

        // Control flags
        tty.c_cflag |= CREAD | CLOCAL; // Enable receiver, ignore modem control lines

        // Input flags - disable software flow control and special handling
        tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Disable XON/XOFF flow control
        tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL); // Raw input

        // Output flags - raw output
        tty.c_oflag &= ~OPOST; // Disable output processing

        // Local flags - raw mode
        tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN); // Raw mode, no echo

        // Set read timeout and minimum bytes
        tty.c_cc[VMIN] = 0; // Non-blocking read
        tty.c_cc[VTIME] = 10; // 1 second timeout (in deciseconds)

        // Apply settings
        if (tcsetattr(port_fd_, TCSANOW, &tty) != 0) {
            return Result<void>::error("Failed to set terminal attributes: " + std::string(strerror(errno)));
        }

        LOG_DEBUG("UART configured: {}bps, {}{}{}", baud_rate, data_bits, parity, stop_bits);
        return Result<void>::success();
    }

    Result<void> Uart::write(std::span<const std::byte> data) {
        if (!isOpen()) {
            return Result<void>::error("UART device is not open");
        }

        if (data.empty()) {
            return Result<void>::success();
        }

        const auto bytes_written = ::write(port_fd_, data.data(), data.size());
        if (bytes_written < 0) {
            return Result<void>::error("Failed to write to UART: " + std::string(strerror(errno)));
        }

        return Result<void>::success();
    }

    Result<std::vector<std::byte>> Uart::read() {
        if (!isOpen()) {
            return Result<std::vector<std::byte>>::error("UART device is not open");
        }

        constexpr size_t MAX_BYTES = 1024;
        constexpr timeval TIMEOUT_DEFAULT{10, 0}; // Long timeout for commands like go to wide/narrow

        while (true) {
            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(port_fd_, &read_fds);

            // reinitialize timeout each select call because select may modify it
            timeval timeout = TIMEOUT_DEFAULT;

            const auto select_result = ::select(port_fd_ + 1, &read_fds, nullptr, nullptr, &timeout);
            if (select_result < 0) {
                if (errno == EINTR) {
                    continue; // interrupted by signal, retry
                }
                return Result<std::vector<std::byte>>::error(std::string("Select failed: ") + std::strerror(errno));
            }
            if (select_result == 0) {
                return Result<std::vector<std::byte>>::error("Read timeout");
            }

            if (!FD_ISSET(port_fd_, &read_fds)) {
                return Result<std::vector<std::byte>>::error("Select returned without UART readiness");
            }

            std::vector<std::byte> buffer(MAX_BYTES);
            const ssize_t bytes_read = ::read(port_fd_, buffer.data(), buffer.size());
            if (bytes_read < 0) {
                if (errno == EINTR) {
                    continue; // interrupted, retry
                }
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // no data available now, loop to wait again
                    continue;
                }
                return Result<std::vector<std::byte>>::error(
                    std::string("Failed to read from UART: ") + std::strerror(errno));
            }

            if (bytes_read == 0) {
                // EOF / device closed
                return Result<std::vector<std::byte>>::error("UART device closed");
            }

            buffer.resize(static_cast<size_t>(bytes_read));
            return Result<std::vector<std::byte>>::success(std::move(buffer));
        }
    }

    bool Uart::isOpen() const {
        return port_fd_ > 0;
    }
}