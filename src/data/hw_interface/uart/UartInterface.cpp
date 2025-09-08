#include "UartInterface.h"

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#include <cerrno>
#include <cstring>
#include <utility>

#include "common/Logger/Logger.h"

namespace camera_service::data::uart {

UartInterface::UartInterface(std::string  device_path)
    : device_path_(std::move(device_path)), device_type_(detectDeviceType()) {
}

UartInterface::~UartInterface() {
    if (isOpen()) {
        auto result = close();
        (void)result; // Acknowledge we're ignoring the result in destructor
    }
}

Result<void> UartInterface::open() {
    if (isOpen()) {
        return Result<void>::error("UART device is already open");
    }

    fd_ = ::open(device_path_.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);

    if (fd_ < 0) {
        return Result<void>::error("Failed to open UART device: " + device_path_ +
                                  " - " + std::string(strerror(errno)));
    }

    return Result<void>::success();
}

Result<void> UartInterface::close() {
    if (!isOpen()) {
        return Result<void>::success();
    }

    if (::close(fd_) < 0) {
        return Result<void>::error("Failed to close UART device: " + std::string(strerror(errno)));
    }

    fd_ = -1;
    return Result<void>::success();
}

Result<void> UartInterface::configure(int baud_rate, int data_bits, int stop_bits, char parity) {
    if (!isOpen()) {
        return Result<void>::error("UART device is not open");
    }

    return setTerminalAttributes(baud_rate, data_bits, stop_bits, parity);
}

Result<size_t> UartInterface::write(const std::vector<char>& data) {
    if (!isOpen()) {
        return Result<size_t>::error("UART device is not open");
    }

    if (data.empty()) {
        return Result<size_t>::success(static_cast<size_t>(0));
    }

    ssize_t bytes_written = ::write(fd_, data.data(), data.size());
    if (bytes_written < 0) {
        return Result<size_t>::error("Failed to write to UART: " + std::string(strerror(errno)));
    }

    return Result<size_t>::success(static_cast<size_t>(bytes_written));
}

Result<std::vector<char>> UartInterface::read() {
    if (!isOpen()) {
        return Result<std::vector<char>>::error("UART device is not open");
    }

    constexpr int timeout_ms = 1000;
    constexpr size_t max_bytes = 1024;

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(fd_, &read_fds);

    struct timeval timeout = {};
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;

    int select_result = select(fd_ + 1, &read_fds, nullptr, nullptr, &timeout);

    if (select_result < 0) {
        return Result<std::vector<char>>::error("Select failed: " + std::string(strerror(errno)));
    }

    if (select_result == 0) {
        return Result<std::vector<char>>::error("Read timeout");
    }

    std::vector<char> buffer(max_bytes);
    ssize_t bytes_read = ::read(fd_, buffer.data(), max_bytes);

    if (bytes_read < 0) {
        return Result<std::vector<char>>::error("Failed to read from UART: " + std::string(strerror(errno)));
    }

    buffer.resize(static_cast<size_t>(bytes_read));
    return Result<std::vector<char>>::success(std::move(buffer));
}

bool UartInterface::isOpen() const {
    return fd_ >= 0;
}

Result<void> UartInterface::setTerminalAttributes(int baud_rate, int data_bits, int stop_bits, char parity) {
    termios tty = {};

    if (tcgetattr(fd_, &tty) != 0) {
        return Result<void>::error("Failed to get terminal attributes: " + std::string(strerror(errno)));
    }

    // For PTS devices, we may need different configuration
    if (device_type_ == DeviceType::PTS) {
        // PTS devices don't need baud rate configuration, but we still set other parameters
        // Configure data bits
        tty.c_cflag &= ~CSIZE;
        switch (data_bits) {
            case 5: tty.c_cflag |= CS5; break;
            case 6: tty.c_cflag |= CS6; break;
            case 7: tty.c_cflag |= CS7; break;
            case 8: tty.c_cflag |= CS8; break;
            default:
                return Result<void>::error("Unsupported data bits: " + std::to_string(data_bits));
        }

        // Configure parity
        switch (parity) {
            case 'N':
            case 'n':
                tty.c_cflag &= ~PARENB;
                break;
            case 'E':
            case 'e':
                tty.c_cflag |= PARENB;
                tty.c_cflag &= ~PARODD;
                break;
            case 'O':
            case 'o':
                tty.c_cflag |= PARENB;
                tty.c_cflag |= PARODD;
                break;
            default:
                return Result<void>::error("Unsupported parity: " + std::string(1, parity));
        }

        // Configure stop bits (though less relevant for PTS)
        if (stop_bits == 1) {
            tty.c_cflag &= ~CSTOPB;
        } else if (stop_bits == 2) {
            tty.c_cflag |= CSTOPB;
        }
    } else {
        // Configure baud rate (only for real UART devices)
        int baud_flag = getBaudRateFlag(baud_rate);
        if (baud_flag == -1) {
            return Result<void>::error("Unsupported baud rate: " + std::to_string(baud_rate));
        }

        cfsetospeed(&tty, static_cast<speed_t>(baud_flag));
        cfsetispeed(&tty, static_cast<speed_t>(baud_flag));

        // Configure data bits
        tty.c_cflag &= ~CSIZE;
        switch (data_bits) {
            case 5: tty.c_cflag |= CS5; break;
            case 6: tty.c_cflag |= CS6; break;
            case 7: tty.c_cflag |= CS7; break;
            case 8: tty.c_cflag |= CS8; break;
            default:
                return Result<void>::error("Unsupported data bits: " + std::to_string(data_bits));
        }

        // Configure stop bits
        if (stop_bits == 1) {
            tty.c_cflag &= ~CSTOPB;
        } else if (stop_bits == 2) {
            tty.c_cflag |= CSTOPB;
        } else {
            return Result<void>::error("Unsupported stop bits: " + std::to_string(stop_bits));
        }

        // Configure parity
        switch (parity) {
            case 'N':
            case 'n':
                tty.c_cflag &= ~PARENB;
                break;
            case 'E':
            case 'e':
                tty.c_cflag |= PARENB;
                tty.c_cflag &= ~PARODD;
                break;
            case 'O':
            case 'o':
                tty.c_cflag |= PARENB;
                tty.c_cflag |= PARODD;
                break;
            default:
                return Result<void>::error("Unsupported parity: " + std::string(1, parity));
        }

        LOG_DEBUG("UART configured: {}bps, {}N{}", baud_rate, data_bits, stop_bits);
    }

    // Common configuration for both UART and PTS
    tty.c_cflag |= CREAD | CLOCAL;
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY | INLCR | ICRNL);
    tty.c_oflag &= ~OPOST;

    // Set timeouts
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 0;

    if (tcsetattr(fd_, TCSANOW, &tty) != 0) {
        return Result<void>::error("Failed to set terminal attributes: " + std::string(strerror(errno)));
    }

    return Result<void>::success();
}

int UartInterface::getBaudRateFlag(int baud_rate) {
    switch (baud_rate) {
        case 9600:   return B9600;
        case 19200:  return B19200;
        case 38400:  return B38400;
        case 57600:  return B57600;
        case 115200: return B115200;
        case 230400: return B230400;
        case 460800: return B460800;
        case 921600: return B921600;
        default:     return -1;
    }
}

DeviceType UartInterface::detectDeviceType() const {
    // Check if the device path indicates a PTS device
    if (device_path_.find("/dev/pts/") == 0) {
        return DeviceType::PTS;
    }
    // Check for other common PTS patterns
    if (device_path_.find("/dev/ptmx") == 0 ||
        device_path_.find("pts") != std::string::npos) {
        return DeviceType::PTS;
    }
    // Default to UART for traditional serial devices
    return DeviceType::UART;
}

}
