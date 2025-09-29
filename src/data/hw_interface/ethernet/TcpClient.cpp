#include "TcpClient.h"

#include <stdexcept>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

namespace camera_service::data {
    struct __attribute__((packed)) ItlHeader {
        uint32_t opcode = 0;
        uint8_t id[4] = {'F', 'R', 'T', 'R'};
        uint16_t length = 0;
        uint16_t counter = 0;
        uint32_t time_stamp = 0;
        uint8_t source = 0;
        uint8_t destination = 0;
        uint16_t checksum = 0;
    };

    struct ItlMessage {
        ItlHeader header;
        std::vector<uint8_t> payload;
    };

    TcpClient::TcpClient(const std::string& device_path) {
        if (device_path.empty()) {
            throw std::invalid_argument("Device path cannot be empty");
        }

        const auto colon_pos = device_path.find(":");
        if (colon_pos == std::string::npos || colon_pos == 0 || colon_pos == device_path.size() - 1) {
            throw std::invalid_argument("Device path must be in format <ip>:<port>");
        }
        ip_ = device_path.substr(0, colon_pos);
        const std::string port_str = device_path.substr(colon_pos + 1);
        try {
            const int port_val = std::stoi(port_str);
            if (port_val < 1 || port_val > 65535) {
                throw std::out_of_range("Port out of range");
            }
            port_ = static_cast<uint16_t>(port_val);
        } catch (const std::exception& e) {
            throw std::invalid_argument(std::string("Invalid port in device path: ") + e.what());
        }
    }

    TcpClient::~TcpClient() {
        if (is_connected_) {
            if (disconnect().isError()) {
                LOG_ERROR("Failed to disconnect TcpClient in destructor"); //FIXME: use layer logger
            }
        }
    }

    Result<void> TcpClient::connect() {
        if (is_connected_) {
            return Result<void>::success();
        }
        socket_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
        if (socket_fd_ < 0) {
            return Result<void>::error("Failed to create socket");
        }
        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port_);
        if (::inet_pton(AF_INET, ip_.c_str(), &server_addr.sin_addr) <= 0) {
            ::close(socket_fd_);
            socket_fd_ = -1;
            return Result<void>::error("Invalid address");
        }
        if (::connect(socket_fd_, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
            ::close(socket_fd_);
            socket_fd_ = -1;
            return Result<void>::error("Connection failed");
        }
        is_connected_ = true;
        return Result<void>::success();
    }

    Result<void> TcpClient::disconnect() {
        if (is_connected_ && socket_fd_ >= 0) {
            ::close(socket_fd_);
            socket_fd_ = -1;
            is_connected_ = false;
        }
        return Result<void>::success();
    }

    Result<void> TcpClient::send(const std::vector<uint8_t>& payload) const {
        if (!is_connected_ || socket_fd_ < 0) {
            return Result<void>::error("Not connected");
        }

        const size_t bytes_to_send = payload.size();
        const auto* data = reinterpret_cast<const char*>(payload.data());
        size_t total_sent = 0;

        while (total_sent < bytes_to_send) {
            const ssize_t sent = ::send(socket_fd_, data + total_sent, bytes_to_send - total_sent, 0);
            if (sent < 0) {
                return Result<void>::error("Send failed");
            }
            total_sent += static_cast<size_t>(sent);
        }
        return Result<void>::success();
    }

    Result<std::vector<uint8_t>> TcpClient::receive() const {
        if (!is_connected_ || socket_fd_ < 0) {
            return Result<std::vector<uint8_t>>::error("Not connected");
        }
        constexpr size_t k_buffer_size = 4096;
        std::vector<uint8_t> response;
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(socket_fd_, &read_fds);
        timeval timeout {1, 0}; // 1 second timeout
        if (const int ready = select(socket_fd_ + 1, &read_fds, nullptr, nullptr, &timeout); ready < 0) {
            return Result<std::vector<uint8_t>>::error("Select failed");
        } else if (ready == 0) {
            // Timeout, no data
            return Result<std::vector<uint8_t>>::error("No answer");
        }
        // Data available
        uint8_t buffer[k_buffer_size] {};
        const ssize_t received = ::recv(socket_fd_, buffer, k_buffer_size, 0);
        if (received < 0) {
            return Result<std::vector<uint8_t>>::error("Receive failed");
        }
        response.insert(response.end(), buffer, buffer + received);

        printMessage(response);

        return Result<std::vector<uint8_t>>::success(response);
    }

    Result<std::vector<uint8_t>> TcpClient::sendPayload(const uint32_t opcode, const std::vector<uint8_t>& payload) const {
        const auto message = createMessage(opcode, payload);
        const auto send_result = send(message);
        if (send_result.isError()) {
            return Result<std::vector<uint8_t>>::error(send_result.error());
        }

        const auto serialized_response = receive();
        if (serialized_response.isError()) {
            return Result<std::vector<uint8_t>>::error(serialized_response.error());
        }

        const auto deserialized_response = deserialize(serialized_response.value());
        if (deserialized_response.isError()) {
            return Result<std::vector<uint8_t>>::error(deserialized_response.error());
        }

        return Result<std::vector<uint8_t>>::success(deserialized_response.value().payload);
    }

    std::vector<uint8_t> TcpClient::createMessage(const uint32_t opcode, const std::vector<uint8_t>& payload) {
        ItlMessage message;
        message.payload = payload;

        message.header.opcode = opcode;
        message.header.length = static_cast<uint16_t>(sizeof(message.header) + payload.size());
        message.header.checksum = calculateMessageChecksum(message);

        printMessage(serialize(message));

        return serialize(message);
    }

    std::vector<uint8_t> TcpClient::serialize(ItlMessage message) {
        auto serialized_message = serializeHeader(message.header);
        serialized_message.insert(serialized_message.end(), message.payload.begin(), message.payload.end());

        return serialized_message;
    }

    std::vector<uint8_t> TcpClient::serializeHeader(const ItlHeader& header) {
        std::vector<uint8_t> serialized_header;

        for (int i = 0; i < sizeof(header.opcode); ++i) {
            serialized_header.push_back(static_cast<uint8_t>((header.opcode >> (i * 8)) & 0xFF));
        }

        for (int i = 0; i < sizeof(header.id); ++i) {
            serialized_header.push_back(header.id[i]);
        }

        for (int i = 0; i < sizeof(header.length); ++i) {
            serialized_header.push_back(static_cast<uint8_t>((header.length >> (i * 8)) & 0xFF));
        }

        for (int i = 0; i < sizeof(header.counter); ++i) {
            serialized_header.push_back(static_cast<uint8_t>((header.counter >> (i * 8)) & 0xFF));
        }

        for (int i = 0; i < sizeof(header.time_stamp); ++i) {
            serialized_header.push_back(static_cast<uint8_t>((header.time_stamp >> (i * 8)) & 0xFF));
        }

        serialized_header.push_back(header.source);

        serialized_header.push_back(header.destination);

        for (int i = 0; i < sizeof(header.checksum); ++i) {
            serialized_header.push_back(static_cast<uint8_t>((header.checksum >> (i * 8)) & 0xFF));
        }

        return serialized_header;
    }

    Result<ItlMessage> TcpClient::deserialize(const std::vector<uint8_t>& data) {
        ItlMessage message;
        if (data.size() < sizeof(ItlHeader)) {
            return Result<ItlMessage>::error("Data too short to contain valid header");
        }

        size_t offset = 0;

        for (int i = 0; i < sizeof(message.header.opcode); ++i) {
            message.header.opcode |= static_cast<uint32_t>(data[offset++]) << (i * 8);
        }
        //TODO: validate received opcode is (xFO | sent opcode)
        if (data[3] != 0xF0) {
            return Result<ItlMessage>::error("Invalid opcode in response");
        }

        for (int i = 0; i < sizeof(message.header.id); ++i) {
            message.header.id[i] = data[offset++];
        }

        for (int i = 0; i < sizeof(message.header.length); ++i) {
            message.header.length |= static_cast<uint16_t>(data[offset++]) << (i * 8);
        }
        if (message.header.length != data.size()) {
            return Result<ItlMessage>::error("Length field does not match actual data size");
        }

        for (int i = 0; i < sizeof(message.header.counter); ++i) {
            message.header.counter |= static_cast<uint16_t>(data[offset++]) << (i * 8);
        }

        for (int i = 0; i < sizeof(message.header.time_stamp); ++i) {
            message.header.time_stamp |= static_cast<uint32_t>(data[offset++]) << (i * 8);
        }

        message.header.source = data[offset++];

        message.header.destination = data[offset++];

        for (int i = 0; i < sizeof(message.header.checksum); ++i) {
            message.header.checksum |= static_cast<uint16_t>(data[offset++]) << (i * 8);
        }

        if (offset < data.size()) {
            message.payload.insert(message.payload.end(), data.begin() + offset, data.end());
        }

        if (!isValidChecksum(message)) {
            return Result<ItlMessage>::error("Received message has invalid checksum");
        }

        return Result<ItlMessage>::success(message);
    }

    void TcpClient::printMessage(const std::vector<uint8_t>& message) {
        std::string buffer = "Message (" + std::to_string(message.size()) + " bytes): [";
        for (size_t i = 0; i < message.size(); ++i) {
            if (i > 0) buffer += " ";
            char hex_buffer[3];
            snprintf(hex_buffer, sizeof(hex_buffer), "%02x", message[i]);
            buffer += hex_buffer;
        }
        buffer += "]";
        LOG_DEBUG("{}", buffer);
    }

    uint16_t TcpClient::calculateXorChecksum(const std::vector<uint8_t>& data) {
        uint16_t checksum = 0;
        for (const auto byte : data) {
            checksum ^= byte;
        }
        return checksum;
    }

    uint16_t TcpClient::calculateMessageChecksum(const ItlMessage& message) {
        const ItlHeader temp_header = message.header;

        auto data_for_checksum = serializeHeader(temp_header);
        data_for_checksum.resize(data_for_checksum.size() - 2);
        data_for_checksum.insert(data_for_checksum.end(), message.payload.begin(), message.payload.end());

        return calculateXorChecksum(data_for_checksum);
    }

    bool TcpClient::isValidChecksum(const ItlMessage& message) {
        ItlMessage temp_message = message;
        temp_message.header.checksum = 0;

        return calculateMessageChecksum(temp_message) == message.header.checksum;
    }
}
