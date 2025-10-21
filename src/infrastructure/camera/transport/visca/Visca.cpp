#include "Visca.h"

#include <algorithm>

namespace camera_service::infrastructure {
    Visca::Visca(std::unique_ptr<ITransport> transport)
        : transport_(std::move(transport)) {}

    std::span<uint8_t> Visca::serialize(ViscaPayload* payload) {
        return {payload->data.data(), payload->size};
    }

    Visca::ViscaPayload Visca::deserialize(std::span<const uint8_t> buffer) {
        ViscaPayload payload{};
        payload.size = buffer.size();
        std::ranges::copy(buffer, payload.data.begin());
        return payload;
    }

    std::vector<uint8_t> Visca::encode(std::span<const uint8_t> payload) const {
        std::vector<uint8_t> frame(payload.size() + 2);
        frame.at(0) = VISCA_START_BYTE;
        frame.at(0) |= (address_ << 4);
        if (broadcast_ > 0) {
            frame.at(0) |= (broadcast_ << 3);
            frame.at(0) &= 0xF8;
        } else {
            frame.at(0) |= cam_address_;
        }

        std::ranges::copy(payload, frame.begin() + 1);

        frame.at(frame.size() - 1) = VISCA_TERMINATOR;
        return frame;
    }

    std::vector<uint8_t> Visca::decode(const std::span<const uint8_t> buffer) const {
        if (buffer.size() < 2) {
            return {};
        }

        if (buffer.front() != VISCA_RESPONSE_START_BYTE && buffer.front() != 0x88) { //TODO: magic number
            return {};
        }

        const auto terminator_it = std::ranges::find(rx_buffer_, static_cast<uint8_t>(VISCA_TERMINATOR));
        if (terminator_it == rx_buffer_.end()) {
            return {};
        }

        buffer_size_ = std::distance(rx_buffer_.begin(), terminator_it) + 1;

        std::vector<uint8_t> payload(buffer_size_ - 3);
        std::ranges::copy(rx_buffer_.begin() + 2, rx_buffer_.begin() + buffer_size_ - 1, payload.begin());

        return payload;
    }

    Result<void> Visca::send(ViscaPayload* payload) const {
        const auto serialized_payload = serialize(payload);
        const auto frame = encode(serialized_payload);

        return transport_->write(frame);
    }

    Result<void> Visca::open() const {
        return transport_->open();
    }

    Result<void> Visca::close() const {
        return transport_->close();
    }

    void Visca::appendByte(ViscaPayload* payload, const uint8_t byte) {
        payload->data.at(payload->size++) = byte;
    }

    void Visca::appendAsNibbles(ViscaPayload* payload, const uint16_t value) {
        appendByte(payload, (value & 0xF000) >> 12);
        appendByte(payload, (value & 0x0F00) >> 8);
        appendByte(payload, (value & 0x00F0) >> 4);
        appendByte(payload, (value & 0x000F));
    }

    Result<ResponseType> Visca::getReply() const {
        if (const auto result = transport_->read(rx_buffer_); result.isError()) {
            return Result<ResponseType>::error(result.error());
        }
        auto type = static_cast<ResponseType>(rx_buffer_.at(1) & 0xF0);

        while (type == ResponseType::Ack) {
            if (const auto result = transport_->read(rx_buffer_); result.isError()) {
                return Result<ResponseType>::error(result.error());
            }
            type = static_cast<ResponseType>(rx_buffer_.at(1) & 0xF0); //TODO: payload
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

    Result<Visca::ViscaPayload> Visca::sendAndReceiveReply(ViscaPayload* payload) const {
        if (const auto result = send(payload); result.isError()) {
            return Result<ViscaPayload>::error(result.error());
        }

        if (const auto result = getReply(); result.isError()) {
            return Result<ViscaPayload>::error(result.error());
        } else if (result.value() == ResponseType::Error) {
            return Result<ViscaPayload>::error(getViscaErrorMessage(static_cast<ResultCode>(rx_buffer_.at(2))));
            //TODO: payload
        }

        const auto response_payload = deserialize(decode(rx_buffer_));

        return Result<ViscaPayload>::success(response_payload);
    }

    Result<void> Visca::setAddress() {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_ADDRESS);
        appendByte(&tx_payload, 0x01);

        const auto backup = broadcast_;
        broadcast_ = 1;
        const auto rx_payload = sendAndReceiveReply(&tx_payload);
        if (rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        cam_address_ = get8Bit(rx_payload.value(), 0) - 1;
        broadcast_ = backup;

        return Result<void>::success();
    }

    Result<void> Visca::clear() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, 0x00);
        appendByte(&tx_payload, 0x01);

        if (const auto result = send(&tx_payload); result.isError()) {
            return Result<void>::error(result.error());
        }
        if (const auto result = getReply(); result.isError()) {
            return Result<void>::error(result.error());
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
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_POWER);
        appendByte(&tx_payload, power);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setKeylock(const uint8_t power) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_KEYLOCK);
        appendByte(&tx_payload, power);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setCameraId(const uint16_t id) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_ID);
        appendAsNibbles(&tx_payload, id);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setZoomTele() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_ZOOM);
        appendByte(&tx_payload, VISCA_ZOOM_TELE);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setZoomWide() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_ZOOM);
        appendByte(&tx_payload, VISCA_ZOOM_WIDE);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setZoomStop() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_ZOOM);
        appendByte(&tx_payload, VISCA_ZOOM_STOP);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setZoomTeleSpeed(const uint32_t speed) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_ZOOM);
        appendByte(&tx_payload, VISCA_ZOOM_TELE_SPEED | (speed & 0x7));

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setZoomWideSpeed(const uint32_t speed) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_ZOOM);
        appendByte(&tx_payload, VISCA_ZOOM_WIDE_SPEED | (speed & 0x7));

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setZoomValue(const uint16_t zoom) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_ZOOM_VALUE);
        appendAsNibbles(&tx_payload, zoom);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setZoomAndFocusValue(const uint16_t zoom, const uint16_t focus) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_ZOOM_FOCUS_VALUE);
        appendAsNibbles(&tx_payload, zoom);
        appendAsNibbles(&tx_payload, focus);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setDzoomValue(const uint8_t value) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_DZOOM_VALUE);
        appendAsNibbles(&tx_payload, value);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setDzoomLimit(const uint8_t limit) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_DZOOM_LIMIT);
        appendByte(&tx_payload, limit);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setDzoomMode(const uint8_t power) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_DZOOM_MODE);
        appendByte(&tx_payload, power);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setFocusFar() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_FOCUS);
        appendByte(&tx_payload, VISCA_FOCUS_FAR);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setFocusNear() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_FOCUS);
        appendByte(&tx_payload, VISCA_FOCUS_NEAR);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setFocusStop() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_FOCUS);
        appendByte(&tx_payload, VISCA_FOCUS_STOP);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setFocusFarSpeed(const uint32_t speed) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_FOCUS);
        appendByte(&tx_payload, VISCA_FOCUS_FAR_SPEED | (speed & 0x7));

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setFocusNearSpeed(const uint32_t speed) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_FOCUS);
        appendByte(&tx_payload, VISCA_FOCUS_NEAR_SPEED | (speed & 0x7));

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setFocusValue(const uint16_t focus) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_FOCUS_VALUE);
        appendAsNibbles(&tx_payload, focus);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setFocusAuto(const bool on) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_FOCUS_AUTO);
        appendByte(&tx_payload, on ? VISCA_ON : VISCA_OFF);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setFocusOnePush() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_FOCUS_ONE_PUSH);
        appendByte(&tx_payload, VISCA_FOCUS_ONE_PUSH_TRIG);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setFocusInfinity() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_FOCUS_ONE_PUSH);
        appendByte(&tx_payload, VISCA_FOCUS_ONE_PUSH_INF);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setFocusAutosenseHigh() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_FOCUS_AUTO_SENSE);
        appendByte(&tx_payload, VISCA_FOCUS_AUTO_SENSE_HIGH);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setFocusAutosenseLow() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_FOCUS_AUTO_SENSE);
        appendByte(&tx_payload, VISCA_FOCUS_AUTO_SENSE_LOW);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setFocusNearLimit(const uint16_t limit) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_FOCUS_NEAR_LIMIT);
        appendAsNibbles(&tx_payload, limit);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setWhitebalMode(const uint8_t mode) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_WB);
        appendByte(&tx_payload, mode);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setWhitebalOnePush() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_WB_TRIGGER);
        appendByte(&tx_payload, VISCA_WB_ONE_PUSH_TRIG);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setRgainUp() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_RGAIN);
        appendByte(&tx_payload, VISCA_UP);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setRgainDown() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_RGAIN);
        appendByte(&tx_payload, VISCA_DOWN);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setRgainReset() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_RGAIN);
        appendByte(&tx_payload, VISCA_RESET);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setRgainValue(const uint8_t value) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_RGAIN_VALUE);
        appendAsNibbles(&tx_payload, value);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setBgainUp() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_BGAIN);
        appendByte(&tx_payload, VISCA_UP);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setBgainDown() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_BGAIN);
        appendByte(&tx_payload, VISCA_DOWN);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setBgainReset() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_BGAIN);
        appendByte(&tx_payload, VISCA_RESET);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setBgainValue(const uint8_t value) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_BGAIN_VALUE);
        appendAsNibbles(&tx_payload, value);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setShutterUp() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_SHUTTER);
        appendByte(&tx_payload, VISCA_UP);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setShutterDown() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_SHUTTER);
        appendByte(&tx_payload, VISCA_DOWN);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setShutterReset() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_SHUTTER);
        appendByte(&tx_payload, VISCA_RESET);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setShutterValue(const uint8_t value) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_SHUTTER_VALUE);
        appendAsNibbles(&tx_payload, value);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setIrisUp() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_IRIS);
        appendByte(&tx_payload, VISCA_UP);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setIrisDown() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_IRIS);
        appendByte(&tx_payload, VISCA_DOWN);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setIrisReset() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_IRIS);
        appendByte(&tx_payload, VISCA_RESET);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setIrisValue(const uint8_t value) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_IRIS_VALUE);
        appendAsNibbles(&tx_payload, value);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setGainUp() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_GAIN);
        appendByte(&tx_payload, VISCA_UP);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setGainDown() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_GAIN);
        appendByte(&tx_payload, VISCA_DOWN);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setGainReset() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_GAIN);
        appendByte(&tx_payload, VISCA_RESET);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setGainValue(const uint8_t value) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_GAIN_VALUE);
        appendAsNibbles(&tx_payload, value);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setBrightUp() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_BRIGHT);
        appendByte(&tx_payload, VISCA_UP);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setBrightDown() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_BRIGHT);
        appendByte(&tx_payload, VISCA_DOWN);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setBrightReset() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_BRIGHT);
        appendByte(&tx_payload, VISCA_RESET);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setBrightValue(const uint16_t value) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_BRIGHT_VALUE);
        appendAsNibbles(&tx_payload, value);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setApertureUp() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_APERTURE);
        appendByte(&tx_payload, VISCA_UP);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setApertureDown() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_APERTURE);
        appendByte(&tx_payload, VISCA_DOWN);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setApertureReset() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_APERTURE);
        appendByte(&tx_payload, VISCA_RESET);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setApertureValue(const uint8_t value) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_APERTURE_VALUE);
        appendAsNibbles(&tx_payload, value);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setExpCompUp() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_EXP_COMP);
        appendByte(&tx_payload, VISCA_UP);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setExpCompDown() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_EXP_COMP);
        appendByte(&tx_payload, VISCA_DOWN);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setExpCompReset() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_EXP_COMP);
        appendByte(&tx_payload, VISCA_RESET);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setExpCompValue(const uint8_t value) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_EXP_COMP_VALUE);
        appendAsNibbles(&tx_payload, value);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setExpCompPower(const uint8_t power) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_EXP_COMP_POWER);
        appendByte(&tx_payload, power);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setAutoExpMode(const uint8_t mode) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_AUTO_EXP);
        appendByte(&tx_payload, mode);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setSlowShutterAuto(const uint8_t power) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_SLOW_SHUTTER);
        appendByte(&tx_payload, power);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setBacklightComp(const bool on) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_BACKLIGHT_COMP);
        if (on) {
            appendByte(&tx_payload, VISCA_ON);
        } else {
            appendByte(&tx_payload, VISCA_OFF);
        }

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setZeroLuxShot(const uint8_t power) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_ZERO_LUX);
        appendByte(&tx_payload, power);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setIrLed(const uint8_t power) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_IR_LED);
        appendByte(&tx_payload, power);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setWideMode(const uint8_t mode) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_WIDE_MODE);
        appendByte(&tx_payload, mode);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setMirror(const uint8_t power) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_MIRROR);
        appendByte(&tx_payload, power);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setFreeze(const uint8_t power) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_FREEZE);
        appendByte(&tx_payload, power);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setPictureEffect(const uint8_t mode) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_PICTURE_EFFECT);
        appendByte(&tx_payload, mode);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setDigitalEffect(const uint8_t mode) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_DIGITAL_EFFECT);
        appendByte(&tx_payload, mode);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setDigitalEffectLevel(const uint8_t level) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_DIGITAL_EFFECT_LEVEL);
        appendByte(&tx_payload, level);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setCamStabilizer(const bool power) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_CAM_STABILIZER);

        if (power) {
            appendByte(&tx_payload, VISCA_ON);
        } else {
            appendByte(&tx_payload, VISCA_OFF);
        }

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::memorySet(const uint8_t channel) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_MEMORY);
        appendByte(&tx_payload, VISCA_MEMORY_SET);
        appendByte(&tx_payload, channel);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::memoryRecall(const uint8_t channel) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_MEMORY);
        appendByte(&tx_payload, VISCA_MEMORY_RECALL);
        appendByte(&tx_payload, channel);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::memoryReset(const uint8_t channel) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_MEMORY);
        appendByte(&tx_payload, VISCA_MEMORY_RESET);
        appendByte(&tx_payload, channel);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setDisplay(const uint8_t power) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_DISPLAY);
        appendByte(&tx_payload, power);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setDateTime(const uint16_t year, const uint16_t month, const uint16_t day, const uint16_t hour,
                                    const uint16_t minute) const {
        if (month < 1 || month > 12 || day < 1 || day > 31 || hour > 23 || minute > 59) {
            return Result<void>::error("Invalid input");
        }

        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_DATE_TIME_SET);
        appendByte(&tx_payload, year / 10);
        appendByte(&tx_payload, year - 10 * (year / 10));
        appendByte(&tx_payload, month / 10);
        appendByte(&tx_payload, month - 10 * (month / 10));
        appendByte(&tx_payload, day / 10);
        appendByte(&tx_payload, day - 10 * (day / 10));
        appendByte(&tx_payload, hour / 10);
        appendByte(&tx_payload, hour - 10 * (hour / 10));
        appendByte(&tx_payload, minute / 10);
        appendByte(&tx_payload, minute - 10 * (minute / 10));

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setDateDisplay(const uint8_t power) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_DATE_DISPLAY);
        appendByte(&tx_payload, power);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setTimeDisplay(const uint8_t power) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_TIME_DISPLAY);
        appendByte(&tx_payload, power);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setTitleDisplay(const uint8_t power) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_TITLE_DISPLAY);
        appendByte(&tx_payload, power);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setTitleClear() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_TITLE_DISPLAY);
        appendByte(&tx_payload, VISCA_TITLE_DISPLAY_CLEAR);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setTitleParams(const ViscaTitleData* title) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_TITLE_SET);
        appendByte(&tx_payload, VISCA_TITLE_SET_PARAMS);
        appendByte(&tx_payload, title->vposition);
        appendByte(&tx_payload, title->hposition);
        appendByte(&tx_payload, title->color);
        appendByte(&tx_payload, title->blink);
        appendByte(&tx_payload, 0);
        appendByte(&tx_payload, 0);
        appendByte(&tx_payload, 0);
        appendByte(&tx_payload, 0);
        appendByte(&tx_payload, 0);
        appendByte(&tx_payload, 0);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setTitle(const ViscaTitleData* title) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_TITLE_SET);
        appendByte(&tx_payload, VISCA_TITLE_SET_PART1);

        for (auto i = 0; i < 10; i++) {
            appendByte(&tx_payload, title->title.at(i));
        }

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_TITLE_SET);
        appendByte(&tx_payload, VISCA_TITLE_SET_PART2);

        for (auto i = 0; i < 10; i++) {
            appendByte(&tx_payload, title->title.at(i + 10));
        }

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setSpotAeOn() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_SPOT_AE);
        appendByte(&tx_payload, VISCA_ON);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setSpotAeOff() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_SPOT_AE);
        appendByte(&tx_payload, VISCA_OFF);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setSpotAePosition(const uint8_t x_position, const uint8_t y_position) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_SPOT_AE_POSITION);
        appendByte(&tx_payload, (x_position & 0xF0) >> 4);
        appendByte(&tx_payload, (x_position & 0x0F));
        appendByte(&tx_payload, (y_position & 0xF0) >> 4);
        appendByte(&tx_payload, (y_position & 0x0F));

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<std::string_view> Visca::getCameraInfo() {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_INQUIRY);
        appendByte(&tx_payload, VISCA_CATEGORY_INTERFACE);
        appendByte(&tx_payload, VISCA_DEVICE_INFO);

        const auto rx_payload = sendAndReceiveReply(&tx_payload);
        if (rx_payload.isError()) {
            return Result<std::string_view>::error(rx_payload.error());
        }

        const auto vendor = get16Bit(rx_payload.value(), 0);
        const auto vendor_str = getCameraVendor(vendor);
        const auto model = get16Bit(rx_payload.value(), 2);
        const auto model_str = getCameraModel(model);

        if (vendor_str == "Unknown" || model_str == "Unknown") {
            return Result<std::string_view>::error("Unknown camera");
        }

        const auto rom_version = get16Bit(rx_payload.value(), 4);
        const auto socket_num = get8Bit(rx_payload.value(), 6);

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
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&tx_payload, VISCA_IRRECEIVE);
        appendByte(&tx_payload, VISCA_ON);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setIrreceiveOff() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&tx_payload, VISCA_IRRECEIVE);
        appendByte(&tx_payload, VISCA_OFF);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setIrreceiveOnoff() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&tx_payload, VISCA_IRRECEIVE);
        appendByte(&tx_payload, VISCA_IRRECEIVE_ONOFF);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setPanTiltUp(const uint8_t pan_speed, const uint8_t tilt_speed) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&tx_payload, VISCA_PT_DRIVE);
        appendByte(&tx_payload, pan_speed);
        appendByte(&tx_payload, tilt_speed);
        appendByte(&tx_payload, VISCA_PT_DRIVE_HORIZ_STOP);
        appendByte(&tx_payload, VISCA_PT_DRIVE_VERT_UP);
        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setPanTiltDown(const uint8_t pan_speed, const uint8_t tilt_speed) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&tx_payload, VISCA_PT_DRIVE);
        appendByte(&tx_payload, pan_speed);
        appendByte(&tx_payload, tilt_speed);
        appendByte(&tx_payload, VISCA_PT_DRIVE_HORIZ_STOP);
        appendByte(&tx_payload, VISCA_PT_DRIVE_VERT_DOWN);
        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setPanTiltLeft(const uint8_t pan_speed, const uint8_t tilt_speed) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&tx_payload, VISCA_PT_DRIVE);
        appendByte(&tx_payload, pan_speed);
        appendByte(&tx_payload, tilt_speed);
        appendByte(&tx_payload, VISCA_PT_DRIVE_HORIZ_LEFT);
        appendByte(&tx_payload, VISCA_PT_DRIVE_VERT_STOP);
        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setPanTiltRight(const uint8_t pan_speed, const uint8_t tilt_speed) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&tx_payload, VISCA_PT_DRIVE);
        appendByte(&tx_payload, pan_speed);
        appendByte(&tx_payload, tilt_speed);
        appendByte(&tx_payload, VISCA_PT_DRIVE_HORIZ_RIGHT);
        appendByte(&tx_payload, VISCA_PT_DRIVE_VERT_STOP);
        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setPanTiltUpleft(const uint8_t pan_speed, const uint8_t tilt_speed) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&tx_payload, VISCA_PT_DRIVE);
        appendByte(&tx_payload, pan_speed);
        appendByte(&tx_payload, tilt_speed);
        appendByte(&tx_payload, VISCA_PT_DRIVE_HORIZ_LEFT);
        appendByte(&tx_payload, VISCA_PT_DRIVE_VERT_UP);
        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setPanTiltUpright(const uint8_t pan_speed, const uint8_t tilt_speed) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&tx_payload, VISCA_PT_DRIVE);
        appendByte(&tx_payload, pan_speed);
        appendByte(&tx_payload, tilt_speed);
        appendByte(&tx_payload, VISCA_PT_DRIVE_HORIZ_RIGHT);
        appendByte(&tx_payload, VISCA_PT_DRIVE_VERT_UP);
        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setPanTiltDownleft(const uint8_t pan_speed, const uint8_t tilt_speed) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&tx_payload, VISCA_PT_DRIVE);
        appendByte(&tx_payload, pan_speed);
        appendByte(&tx_payload, tilt_speed);
        appendByte(&tx_payload, VISCA_PT_DRIVE_HORIZ_LEFT);
        appendByte(&tx_payload, VISCA_PT_DRIVE_VERT_DOWN);
        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setPanTiltDownright(const uint8_t pan_speed, const uint8_t tilt_speed) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&tx_payload, VISCA_PT_DRIVE);
        appendByte(&tx_payload, pan_speed);
        appendByte(&tx_payload, tilt_speed);
        appendByte(&tx_payload, VISCA_PT_DRIVE_HORIZ_RIGHT);
        appendByte(&tx_payload, VISCA_PT_DRIVE_VERT_DOWN);
        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setPanTiltStop(const uint8_t pan_speed, const uint8_t tilt_speed) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&tx_payload, VISCA_PT_DRIVE);
        appendByte(&tx_payload, pan_speed);
        appendByte(&tx_payload, tilt_speed);
        appendByte(&tx_payload, VISCA_PT_DRIVE_HORIZ_STOP);
        appendByte(&tx_payload, VISCA_PT_DRIVE_VERT_STOP);
        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setPanTiltAbsolutePosition(const uint8_t pan_speed, const uint8_t tilt_speed,
                                                   const uint16_t pan_pos, const uint16_t tilt_pos) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&tx_payload, VISCA_PT_ABSOLUTE_POSITION);
        appendByte(&tx_payload, pan_speed);
        appendByte(&tx_payload, tilt_speed);
        appendAsNibbles(&tx_payload, pan_pos);
        appendAsNibbles(&tx_payload, tilt_pos);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setPanTiltRelativePosition(const uint8_t pan_speed, const uint8_t tilt_speed,
                                                   const uint16_t pan_pos, const uint16_t tilt_pos) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&tx_payload, VISCA_PT_RELATIVE_POSITION);
        appendByte(&tx_payload, pan_speed);
        appendByte(&tx_payload, tilt_speed);

        appendByte(&tx_payload, (pan_pos & 0xf000) >> 12);
        appendByte(&tx_payload, (pan_pos & 0x0f00) >> 8);
        appendByte(&tx_payload, (pan_pos & 0x00f0) >> 4);
        appendByte(&tx_payload, pan_pos & 0x000f);

        appendByte(&tx_payload, (tilt_pos & 0xf000) >> 12);
        appendByte(&tx_payload, (tilt_pos & 0x0f00) >> 8);
        appendByte(&tx_payload, (tilt_pos & 0x00f0) >> 4);
        appendByte(&tx_payload, tilt_pos & 0x000f);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setPanTiltHome() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&tx_payload, VISCA_PT_HOME);
        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setPanTiltReset() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&tx_payload, VISCA_PT_RESET);
        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setPanTiltLimitUpright(const uint16_t pan_limit, const uint16_t tilt_limit) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&tx_payload, VISCA_PT_LIMITSET);
        appendByte(&tx_payload, VISCA_PT_LIMITSET_SET);
        appendByte(&tx_payload, VISCA_PT_LIMITSET_SET_UR);
        appendAsNibbles(&tx_payload, pan_limit);
        appendAsNibbles(&tx_payload, tilt_limit);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setPanTiltLimitDownleft(const uint16_t pan_limit, const uint16_t tilt_limit) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&tx_payload, VISCA_PT_LIMITSET);
        appendByte(&tx_payload, VISCA_PT_LIMITSET_SET);
        appendByte(&tx_payload, VISCA_PT_LIMITSET_SET_DL);
        appendAsNibbles(&tx_payload, pan_limit);
        appendAsNibbles(&tx_payload, tilt_limit);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setPanTiltLimitDownleftClear() const {
        ViscaPayload tx_payload{};

        constexpr uint16_t pan_lmit = 0x7fff;
        constexpr uint16_t tilt_limit = 0x7fff;

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&tx_payload, VISCA_PT_LIMITSET);
        appendByte(&tx_payload, VISCA_PT_LIMITSET_CLEAR);
        appendByte(&tx_payload, VISCA_PT_LIMITSET_SET_DL);
        appendAsNibbles(&tx_payload, pan_lmit);
        appendAsNibbles(&tx_payload, tilt_limit);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setPanTiltLimitUprightClear() const {
        ViscaPayload tx_payload{};

        constexpr uint16_t pan_limit = 0x7fff;
        constexpr uint16_t tilt_limit = 0x7fff;

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&tx_payload, VISCA_PT_LIMITSET);
        appendByte(&tx_payload, VISCA_PT_LIMITSET_CLEAR);
        appendByte(&tx_payload, VISCA_PT_LIMITSET_SET_UR);
        appendAsNibbles(&tx_payload, pan_limit);
        appendAsNibbles(&tx_payload, tilt_limit);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setDatascreenOn() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&tx_payload, VISCA_PT_DATASCREEN);
        appendByte(&tx_payload, VISCA_ON);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setDatascreenOff() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&tx_payload, VISCA_PT_DATASCREEN);
        appendByte(&tx_payload, VISCA_OFF);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setDatascreenOnoff() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&tx_payload, VISCA_PT_DATASCREEN);
        appendByte(&tx_payload, VISCA_PT_DATASCREEN_ONOFF);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<uint8_t> Visca::getPower() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_INQUIRY);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_POWER);
        const auto rx_payload = sendAndReceiveReply(&tx_payload);
        if (rx_payload.isError()) {
            return Result<uint8_t>::error(rx_payload.error());
        }
        return Result<uint8_t>::success(get8Bit(rx_payload.value(), 0));
    }

    Result<uint8_t> Visca::getDzoomValue() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_INQUIRY);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_DZOOM);

        const auto rx_payload = sendAndReceiveReply(&tx_payload);
        if (rx_payload.isError()) {
            return Result<uint8_t>::error(rx_payload.error());
        }
        return Result<uint8_t>::success(get8Bit(rx_payload.value(), 0));
    }

    Result<uint8_t> Visca::getDzoomLimit() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_INQUIRY);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_DZOOM_LIMIT);
        const auto rx_payload = sendAndReceiveReply(&tx_payload);
        if (rx_payload.isError()) {
            return Result<uint8_t>::error(rx_payload.error());
        }
        return Result<uint8_t>::success(get8Bit(rx_payload.value(), 0));
    }

    Result<uint16_t> Visca::getZoomValue() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_INQUIRY);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_ZOOM_VALUE);

        const auto rx_payload = sendAndReceiveReply(&tx_payload);
        if (rx_payload.isError()) {
            return Result<uint16_t>::error(rx_payload.error());
        }
        return Result<uint16_t>::success(get16BitFromNibbles(rx_payload.value(), 0));
    }

    Result<bool> Visca::getFocusAuto() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_INQUIRY);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_FOCUS_AUTO);

        const auto rx_payload = sendAndReceiveReply(&tx_payload);
        if (rx_payload.isError()) {
            return Result<bool>::error(rx_payload.error());
        }

        if (get8Bit(rx_payload.value(), 0) == VISCA_OFF) {
            return Result<bool>::success(false);
        }
        return Result<bool>::success(true);
    }

    Result<uint16_t> Visca::getFocusValue() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_INQUIRY);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_FOCUS_VALUE);

        const auto rx_payload = sendAndReceiveReply(&tx_payload);
        if (rx_payload.isError()) {
            return Result<uint16_t>::error(rx_payload.error());
        }
        return Result<uint16_t>::success(get16BitFromNibbles(rx_payload.value(), 0));
    }

    Result<uint8_t> Visca::getFocusAutoSense() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_INQUIRY);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_FOCUS_AUTO_SENSE);

        const auto rx_payload = sendAndReceiveReply(&tx_payload);
        if (rx_payload.isError()) {
            return Result<uint8_t>::error(rx_payload.error());
        }
        return Result<uint8_t>::success(get8Bit(rx_payload.value(), 0));
    }

    Result<uint16_t> Visca::getFocusNearLimit() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_INQUIRY);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_FOCUS_NEAR_LIMIT);

        const auto rx_payload = sendAndReceiveReply(&tx_payload);
        if (rx_payload.isError()) {
            return Result<uint16_t>::error(rx_payload.error());
        }
        return Result<uint16_t>::success(get16BitFromNibbles(rx_payload.value(), 0));
    }

    Result<uint8_t> Visca::getWhitebalMode() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_INQUIRY);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_WB);

        const auto rx_payload = sendAndReceiveReply(&tx_payload);
        if (rx_payload.isError()) {
            return Result<uint8_t>::error(rx_payload.error());
        }
        return Result<uint8_t>::success(get8Bit(rx_payload.value(), 0));
    }

    Result<uint8_t> Visca::getRgainValue() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_INQUIRY);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_RGAIN_VALUE);

        const auto rx_payload = sendAndReceiveReply(&tx_payload);
        if (rx_payload.isError()) {
            return Result<uint8_t>::error(rx_payload.error());
        }
        return Result<uint8_t>::success(get8Bit(rx_payload.value(), 0));
    }

    Result<uint8_t> Visca::getBgainValue() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_INQUIRY);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_BGAIN_VALUE);

        const auto rx_payload = sendAndReceiveReply(&tx_payload);
        if (rx_payload.isError()) {
            return Result<uint8_t>::error(rx_payload.error());
        }
        return Result<uint8_t>::success(get8Bit(rx_payload.value(), 0));
    }

    Result<uint8_t> Visca::getAutoExpMode() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_INQUIRY);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_AUTO_EXP);

        const auto rx_payload = sendAndReceiveReply(&tx_payload);
        if (rx_payload.isError()) {
            return Result<uint8_t>::error(rx_payload.error());
        }
        return Result<uint8_t>::success(get8Bit(rx_payload.value(), 0));
    }

    Result<uint8_t> Visca::getSlowShutterAuto() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_INQUIRY);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_SLOW_SHUTTER);

        const auto rx_payload = sendAndReceiveReply(&tx_payload);
        if (rx_payload.isError()) {
            return Result<uint8_t>::error(rx_payload.error());
        }
        return Result<uint8_t>::success(get8Bit(rx_payload.value(), 0));
    }

    Result<uint8_t> Visca::getShutterValue() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_INQUIRY);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_SHUTTER_VALUE);

        const auto rx_payload = sendAndReceiveReply(&tx_payload);
        if (rx_payload.isError()) {
            return Result<uint8_t>::error(rx_payload.error());
        }
        return Result<uint8_t>::success(static_cast<uint8_t>(get16BitFromNibbles(rx_payload.value(), 0)));
    }

    Result<uint8_t> Visca::getIrisValue() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_INQUIRY);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_IRIS_VALUE);

        const auto rx_payload = sendAndReceiveReply(&tx_payload);
        if (rx_payload.isError()) {
            return Result<uint8_t>::error(rx_payload.error());
        }
        return Result<uint8_t>::success(static_cast<uint8_t>(get16BitFromNibbles(rx_payload.value(), 0)));
    }

    Result<uint8_t> Visca::getGainValue() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_INQUIRY);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_GAIN_VALUE);

        const auto rx_payload = sendAndReceiveReply(&tx_payload);
        if (rx_payload.isError()) {
            return Result<uint8_t>::error(rx_payload.error());
        }
        return Result<uint8_t>::success(static_cast<uint8_t>(get16BitFromNibbles(rx_payload.value(), 0)));
    }

    Result<uint16_t> Visca::getBrightValue() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_INQUIRY);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_BRIGHT_VALUE);

        const auto rx_payload = sendAndReceiveReply(&tx_payload);
        if (rx_payload.isError()) {
            return Result<uint16_t>::error(rx_payload.error());
        }
        return Result<uint16_t>::success(get16BitFromNibbles(rx_payload.value(), 0));
    }

    Result<uint8_t> Visca::getExpCompPower() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_INQUIRY);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_EXP_COMP_POWER);

        const auto rx_payload = sendAndReceiveReply(&tx_payload);
        if (rx_payload.isError()) {
            return Result<uint8_t>::error(rx_payload.error());
        }
        return Result<uint8_t>::success(get8Bit(rx_payload.value(), 0));
    }

    Result<uint8_t> Visca::getExpCompValue() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_INQUIRY);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_EXP_COMP_VALUE);

        const auto rx_payload = sendAndReceiveReply(&tx_payload);
        if (rx_payload.isError()) {
            return Result<uint8_t>::error(rx_payload.error());
        }
        return Result<uint8_t>::success(static_cast<uint8_t>(get16BitFromNibbles(rx_payload.value(), 0)));
    }

    Result<bool> Visca::getBacklightComp() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_INQUIRY);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_BACKLIGHT_COMP);

        const auto rx_payload = sendAndReceiveReply(&tx_payload);
        if (rx_payload.isError()) {
            return Result<bool>::error(rx_payload.error());
        }

        if (get8Bit(rx_payload.value(), 0) == VISCA_OFF) {
            return Result<bool>::success(false);
        }
        return Result<bool>::success(true);
    }

    Result<uint8_t> Visca::getApertureValue() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_INQUIRY);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_APERTURE_VALUE);

        const auto rx_payload = sendAndReceiveReply(&tx_payload);
        if (rx_payload.isError()) {
            return Result<uint8_t>::error(rx_payload.error());
        }
        return Result<uint8_t>::success(static_cast<uint8_t>(get16BitFromNibbles(rx_payload.value(), 0)));
    }

    Result<uint8_t> Visca::getZeroLuxShot() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_INQUIRY);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_ZERO_LUX);

        const auto rx_payload = sendAndReceiveReply(&tx_payload);
        if (rx_payload.isError()) {
            return Result<uint8_t>::error(rx_payload.error());
        }
        return Result<uint8_t>::success(get8Bit(rx_payload.value(), 0));
    }

    Result<uint8_t> Visca::getIrLed() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_INQUIRY);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_IR_LED);

        const auto rx_payload = sendAndReceiveReply(&tx_payload);
        if (rx_payload.isError()) {
            return Result<uint8_t>::error(rx_payload.error());
        }
        return Result<uint8_t>::success(get8Bit(rx_payload.value(), 0));
    }

    Result<uint8_t> Visca::getWideMode() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_INQUIRY);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_WIDE_MODE);

        const auto rx_payload = sendAndReceiveReply(&tx_payload);
        if (rx_payload.isError()) {
            return Result<uint8_t>::error(rx_payload.error());
        }
        return Result<uint8_t>::success(get8Bit(rx_payload.value(), 0));
    }

    Result<uint8_t> Visca::getMirror() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_INQUIRY);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_MIRROR);

        const auto rx_payload = sendAndReceiveReply(&tx_payload);
        if (rx_payload.isError()) {
            return Result<uint8_t>::error(rx_payload.error());
        }
        return Result<uint8_t>::success(get8Bit(rx_payload.value(), 0));
    }

    Result<uint8_t> Visca::getFreeze() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_INQUIRY);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_FREEZE);

        const auto rx_payload = sendAndReceiveReply(&tx_payload);
        if (rx_payload.isError()) {
            return Result<uint8_t>::error(rx_payload.error());
        }
        return Result<uint8_t>::success(get8Bit(rx_payload.value(), 0));
    }

    Result<uint8_t> Visca::getPictureEffect() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_INQUIRY);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_PICTURE_EFFECT);

        const auto rx_payload = sendAndReceiveReply(&tx_payload);
        if (rx_payload.isError()) {
            return Result<uint8_t>::error(rx_payload.error());
        }
        return Result<uint8_t>::success(get8Bit(rx_payload.value(), 0));
    }

    Result<uint8_t> Visca::getDigitalEffect() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_INQUIRY);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_DIGITAL_EFFECT);

        const auto rx_payload = sendAndReceiveReply(&tx_payload);
        if (rx_payload.isError()) {
            return Result<uint8_t>::error(rx_payload.error());
        }
        return Result<uint8_t>::success(get8Bit(rx_payload.value(), 0));
    }

    Result<uint16_t> Visca::getDigitalEffectLevel() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_INQUIRY);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_DIGITAL_EFFECT_LEVEL);

        const auto rx_payload = sendAndReceiveReply(&tx_payload);
        if (rx_payload.isError()) {
            return Result<uint16_t>::error(rx_payload.error());
        }
        return Result<uint16_t>::success(get16BitFromNibbles(rx_payload.value(), 0));
    }

    Result<uint8_t> Visca::getMemory() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_INQUIRY);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_MEMORY);

        const auto rx_payload = sendAndReceiveReply(&tx_payload);
        if (rx_payload.isError()) {
            return Result<uint8_t>::error(rx_payload.error());
        }
        return Result<uint8_t>::success(get8Bit(rx_payload.value(), 0));
    }

    Result<uint8_t> Visca::getDisplay() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_INQUIRY);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_DISPLAY);

        const auto rx_payload = sendAndReceiveReply(&tx_payload);
        if (rx_payload.isError()) {
            return Result<uint8_t>::error(rx_payload.error());
        }
        return Result<uint8_t>::success(get8Bit(rx_payload.value(), 0));
    }

    Result<uint16_t> Visca::getId() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_INQUIRY);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_ID);

        const auto rx_payload = sendAndReceiveReply(&tx_payload);
        if (rx_payload.isError()) {
            return Result<uint16_t>::error(rx_payload.error());
        }
        return Result<uint16_t>::success(get16BitFromNibbles(rx_payload.value(), 0));
    }

    Result<void> Visca::setRegister(const uint8_t reg_num, const uint8_t reg_val) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_REGISTER_VALUE);
        appendByte(&tx_payload, reg_num);
        appendByte(&tx_payload, (reg_val & 0xF0) >> 4);
        appendByte(&tx_payload, (reg_val & 0x0F));
        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    uint8_t Visca::get8Bit(const ViscaPayload& payload, const size_t index) {
        return static_cast<uint8_t>(payload.data.at(index));
    }

    uint8_t Visca::get8BitFromNibbles(const ViscaPayload& payload, const size_t index) {
        const auto high = static_cast<uint16_t>(payload.data.at(index)) << 4;
        const auto low = static_cast<uint16_t>(payload.data.at(index + 1));
        return static_cast<uint8_t>(high | low);
    }

    uint16_t Visca::get16Bit(const ViscaPayload& payload, const size_t index) {
        const auto high = static_cast<uint16_t>(payload.data.at(index)) << 8;
        const auto low = static_cast<uint16_t>(payload.data.at(index + 1));
        return static_cast<uint16_t>(high | low);
    }

    uint16_t Visca::get16BitFromNibbles(const ViscaPayload& payload, const size_t index) {
        const auto b0 = static_cast<uint16_t>(payload.data.at(index)) << 12;
        const auto b1 = static_cast<uint16_t>(payload.data.at(index + 1)) << 8;
        const auto b2 = static_cast<uint16_t>(payload.data.at(index + 2)) << 4;
        const auto b3 = static_cast<uint16_t>(payload.data.at(index + 3));
        return static_cast<uint16_t>(b0 | b1 | b2 | b3);
    }

    Result<uint8_t> Visca::getVideoSystem() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_INQUIRY);
        appendByte(&tx_payload, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&tx_payload, VISCA_PT_VIDEOSYSTEM_INQ);

        const auto rx_payload = sendAndReceiveReply(&tx_payload);
        if (rx_payload.isError()) {
            return Result<uint8_t>::error(rx_payload.error());
        }
        return Result<uint8_t>::success(get8Bit(rx_payload.value(), 0));
    }

    Result<uint16_t> Visca::getPanTiltMode() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_INQUIRY);
        appendByte(&tx_payload, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&tx_payload, VISCA_PT_MODE_INQ);

        const auto rx_payload = sendAndReceiveReply(&tx_payload);
        if (rx_payload.isError()) {
            return Result<uint16_t>::error(rx_payload.error());
        }
        const uint16_t status = get16Bit(rx_payload.value(), 1);
        return Result<uint16_t>::success(status);
    }

    Result<std::pair<uint8_t, uint8_t>> Visca::getPanTiltMaxspeed() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_INQUIRY);
        appendByte(&tx_payload, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&tx_payload, VISCA_PT_MAXSPEED_INQ);

        const auto rx_payload = sendAndReceiveReply(&tx_payload);
        if (rx_payload.isError()) {
            return Result<std::pair<uint8_t, uint8_t>>::error(rx_payload.error());
        }
        const uint8_t max_pan_speed = get8Bit(rx_payload.value(), 0);
        const uint8_t max_tilt_speed = get8Bit(rx_payload.value(), 1);
        return Result<std::pair<uint8_t, uint8_t>>::success({max_pan_speed, max_tilt_speed});
    }

    Result<std::pair<uint16_t, uint16_t>> Visca::getPanTiltPosition() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_INQUIRY);
        appendByte(&tx_payload, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&tx_payload, VISCA_PT_POSITION_INQ);

        const auto rx_payload = sendAndReceiveReply(&tx_payload);
        if (rx_payload.isError()) {
            return Result<std::pair<uint16_t, uint16_t>>::error(rx_payload.error());
        }
        const auto pan_position = get16BitFromNibbles(rx_payload.value(), 0);
        const auto tilt_position = get16BitFromNibbles(rx_payload.value(), 4);

        return Result<std::pair<uint16_t, uint16_t>>::success({pan_position, tilt_position});
    }

    Result<uint8_t> Visca::getDatascreen() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_INQUIRY);
        appendByte(&tx_payload, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&tx_payload, VISCA_PT_DATASCREEN_INQ);

        const auto rx_payload = sendAndReceiveReply(&tx_payload);
        if (rx_payload.isError()) {
            return Result<uint8_t>::error(rx_payload.error());
        }
        return Result<uint8_t>::success(get8Bit(rx_payload.value(), 0));
    }

    Result<uint8_t> Visca::getRegister(const uint8_t reg_num) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_INQUIRY);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_REGISTER_VALUE);
        appendByte(&tx_payload, reg_num);

        const auto rx_payload = sendAndReceiveReply(&tx_payload);
        if (rx_payload.isError()) {
            return Result<uint8_t>::error(rx_payload.error());
        }
        const uint8_t reg_val = get8Bit(rx_payload.value(), 0);
        return Result<uint8_t>::success(reg_val);
    }

    /********************************/
    /* SPECIAL FUNCTIONS FOR D30/31 */
    /********************************/

    Result<void> Visca::setWideConLens(const uint8_t power) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA2);
        appendByte(&tx_payload, VISCA_WIDE_CON_LENS);
        appendByte(&tx_payload, VISCA_WIDE_CON_LENS_SET);
        appendByte(&tx_payload, power);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setAtModeOnoff() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA2);
        appendByte(&tx_payload, VISCA_AT_MODE);
        appendByte(&tx_payload, VISCA_AT_ONOFF);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setAtMode(const uint8_t power) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA2);
        appendByte(&tx_payload, VISCA_AT_MODE);
        appendByte(&tx_payload, power);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setAtAeOnoff() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA2);
        appendByte(&tx_payload, VISCA_AT_AE);
        appendByte(&tx_payload, VISCA_AT_ONOFF);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setAtAe(const uint8_t power) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA2);
        appendByte(&tx_payload, VISCA_AT_AE);
        appendByte(&tx_payload, power);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setAtAutozoomOnoff() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA2);
        appendByte(&tx_payload, VISCA_AT_AUTOZOOM);
        appendByte(&tx_payload, VISCA_AT_ONOFF);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setAtAutozoom(const uint8_t power) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA2);
        appendByte(&tx_payload, VISCA_AT_AUTOZOOM);
        appendByte(&tx_payload, power);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setAtmdFramedisplayOnoff() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA2);
        appendByte(&tx_payload, VISCA_ATMD_FRAMEDISPLAY);
        appendByte(&tx_payload, VISCA_AT_ONOFF);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setAtmdFramedisplay(const uint8_t power) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA2);
        appendByte(&tx_payload, VISCA_ATMD_FRAMEDISPLAY);
        appendByte(&tx_payload, power);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setAtFrameoffsetOnoff() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA2);
        appendByte(&tx_payload, VISCA_AT_FRAMEOFFSET);
        appendByte(&tx_payload, VISCA_AT_ONOFF);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setAtFrameoffset(const uint8_t power) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA2);
        appendByte(&tx_payload, VISCA_AT_FRAMEOFFSET);
        appendByte(&tx_payload, power);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setAtmdStartstop() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA2);
        appendByte(&tx_payload, VISCA_ATMD_STARTSTOP);
        appendByte(&tx_payload, VISCA_AT_ONOFF);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setAtChase(const uint8_t power) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA2);
        appendByte(&tx_payload, VISCA_AT_CHASE);
        appendByte(&tx_payload, power);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setAtChaseNext() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA2);
        appendByte(&tx_payload, VISCA_AT_CHASE);
        appendByte(&tx_payload, VISCA_AT_CHASE_NEXT);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setMdModeOnoff() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA2);
        appendByte(&tx_payload, VISCA_MD_MODE);
        appendByte(&tx_payload, VISCA_MD_ONOFF);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setMdMode(const uint8_t power) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA2);
        appendByte(&tx_payload, VISCA_MD_MODE);
        appendByte(&tx_payload, power);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setMdFrame() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA2);
        appendByte(&tx_payload, VISCA_MD_FRAME);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setMdDetect() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA2);
        appendByte(&tx_payload, VISCA_MD_DETECT);
        appendByte(&tx_payload, VISCA_MD_ONOFF);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setAtEntry(const uint8_t power) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA2);
        appendByte(&tx_payload, VISCA_AT_ENTRY);
        appendByte(&tx_payload, power);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setAtLostinfo() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&tx_payload, VISCA_ATMD_LOSTINFO1);
        appendByte(&tx_payload, VISCA_ATMD_LOSTINFO2);
        appendByte(&tx_payload, VISCA_AT_LOSTINFO);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setMdLostinfo() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&tx_payload, VISCA_ATMD_LOSTINFO1);
        appendByte(&tx_payload, VISCA_ATMD_LOSTINFO2);
        appendByte(&tx_payload, VISCA_MD_LOSTINFO);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setMdAdjustYlevel(const uint8_t power) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA2);
        appendByte(&tx_payload, VISCA_MD_ADJUST_YLEVEL);
        appendByte(&tx_payload, VISCA_MD_ADJUST);
        appendByte(&tx_payload, power);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setMdAdjustHuelevel(const uint8_t power) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA2);
        appendByte(&tx_payload, VISCA_MD_ADJUST_HUELEVEL);
        appendByte(&tx_payload, VISCA_MD_ADJUST);
        appendByte(&tx_payload, power);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setMdAdjustSize(const uint8_t power) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA2);
        appendByte(&tx_payload, VISCA_MD_ADJUST_SIZE);
        appendByte(&tx_payload, VISCA_MD_ADJUST);
        appendByte(&tx_payload, power);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setMdAdjustDisptime(const uint8_t power) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA2);
        appendByte(&tx_payload, VISCA_MD_ADJUST_DISPTIME);
        appendByte(&tx_payload, VISCA_MD_ADJUST);
        appendByte(&tx_payload, power);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setMdAdjustRefmode(const uint8_t power) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA2);
        appendByte(&tx_payload, VISCA_MD_ADJUST_REFMODE);
        appendByte(&tx_payload, power);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setMdAdjustReftime(const uint8_t power) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA2);
        appendByte(&tx_payload, VISCA_MD_REFTIME_QUERY);
        appendByte(&tx_payload, VISCA_MD_ADJUST);
        appendByte(&tx_payload, power);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setMdMeasureMode1Onoff() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA2);
        appendByte(&tx_payload, VISCA_MD_MEASURE_MODE_1);
        appendByte(&tx_payload, VISCA_MD_ONOFF);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setMdMeasureMode1(const uint8_t power) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA2);
        appendByte(&tx_payload, VISCA_MD_MEASURE_MODE_1);
        appendByte(&tx_payload, power);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setMdMeasureMode2Onoff() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA2);
        appendByte(&tx_payload, VISCA_MD_MEASURE_MODE_2);
        appendByte(&tx_payload, VISCA_MD_ONOFF);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> Visca::setMdMeasureMode2(const uint8_t power) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA2);
        appendByte(&tx_payload, VISCA_MD_MEASURE_MODE_2);
        appendByte(&tx_payload, power);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<uint8_t> Visca::getKeylock() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_INQUIRY);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_KEYLOCK);

        const auto rx_payload = sendAndReceiveReply(&tx_payload);
        if (rx_payload.isError()) {
            return Result<uint8_t>::error(rx_payload.error());
        }
        return Result<uint8_t>::success(get8Bit(rx_payload.value(), 0));
    }

    Result<uint8_t> Visca::getWideConLens() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_INQUIRY);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_WIDE_CON_LENS);

        const auto rx_payload = sendAndReceiveReply(&tx_payload);
        if (rx_payload.isError()) {
            return Result<uint8_t>::error(rx_payload.error());
        }
        return Result<uint8_t>::success(get8Bit(rx_payload.value(), 0));
    }

    Result<uint8_t> Visca::getAtmdMode() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_INQUIRY);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA2);
        appendByte(&tx_payload, VISCA_ATMD_MODE);

        const auto rx_payload = sendAndReceiveReply(&tx_payload);
        if (rx_payload.isError()) {
            return Result<uint8_t>::error(rx_payload.error());
        }
        return Result<uint8_t>::success(get8Bit(rx_payload.value(), 0));
    }

    Result<uint16_t> Visca::getAtMode() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_INQUIRY);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA2);
        appendByte(&tx_payload, VISCA_AT_MODE_QUERY);

        const auto rx_payload = sendAndReceiveReply(&tx_payload);
        if (rx_payload.isError()) {
            return Result<uint16_t>::error(rx_payload.error());
        }
        const uint16_t value = get16Bit(rx_payload.value(), 1);
        return Result<uint16_t>::success(value);
    }

    Result<uint8_t> Visca::getAtEntry() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_INQUIRY);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA2);
        appendByte(&tx_payload, VISCA_AT_ENTRY);

        const auto rx_payload = sendAndReceiveReply(&tx_payload);
        if (rx_payload.isError()) {
            return Result<uint8_t>::error(rx_payload.error());
        }
        return Result<uint8_t>::success(get8Bit(rx_payload.value(), 0));
    }

    Result<uint16_t> Visca::getMdMode() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_INQUIRY);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA2);
        appendByte(&tx_payload, VISCA_MD_MODE_QUERY);
        const auto rx_payload = sendAndReceiveReply(&tx_payload);
        if (rx_payload.isError()) {
            return Result<uint16_t>::error(rx_payload.error());
        }
        const uint16_t value = get16Bit(rx_payload.value(), 1);
        return Result<uint16_t>::success(value);
    }

    Result<uint8_t> Visca::getMdYlevel() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_INQUIRY);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA2);
        appendByte(&tx_payload, VISCA_MD_ADJUST_YLEVEL);
        const auto rx_payload = sendAndReceiveReply(&tx_payload);
        if (rx_payload.isError()) {
            return Result<uint8_t>::error(rx_payload.error());
        }
        const uint8_t power = get8BitFromNibbles(rx_payload.value(), 0);
        return Result<uint8_t>::success(power);
    }

    Result<uint8_t> Visca::getMdHuelevel() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_INQUIRY);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA2);
        appendByte(&tx_payload, VISCA_MD_ADJUST_HUELEVEL);
        const auto rx_payload = sendAndReceiveReply(&tx_payload);
        if (rx_payload.isError()) {
            return Result<uint8_t>::error(rx_payload.error());
        }
        const uint8_t power = get8BitFromNibbles(rx_payload.value(), 0);
        return Result<uint8_t>::success(power);
    }

    Result<uint8_t> Visca::getMdSize() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_INQUIRY);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA2);
        appendByte(&tx_payload, VISCA_MD_ADJUST_SIZE);
        const auto rx_payload = sendAndReceiveReply(&tx_payload);
        if (rx_payload.isError()) {
            return Result<uint8_t>::error(rx_payload.error());
        }
        const uint8_t power = get8BitFromNibbles(rx_payload.value(), 0);
        return Result<uint8_t>::success(power);
    }

    Result<uint8_t> Visca::getMdDisptime() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_INQUIRY);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA2);
        appendByte(&tx_payload, VISCA_MD_ADJUST_DISPTIME);

        const auto rx_payload = sendAndReceiveReply(&tx_payload);
        if (rx_payload.isError()) {
            return Result<uint8_t>::error(rx_payload.error());
        }
        const uint8_t power = get8BitFromNibbles(rx_payload.value(), 0);
        return Result<uint8_t>::success(power);
    }

    Result<uint8_t> Visca::getMdRefmode() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_INQUIRY);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA2);
        appendByte(&tx_payload, VISCA_MD_ADJUST_REFMODE);

        const auto rx_payload = sendAndReceiveReply(&tx_payload);
        if (rx_payload.isError()) {
            return Result<uint8_t>::error(rx_payload.error());
        }
        return Result<uint8_t>::success(get8Bit(rx_payload.value(), 0));
    }

    Result<uint8_t> Visca::getMdReftime() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_INQUIRY);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA2);
        appendByte(&tx_payload, VISCA_MD_REFTIME_QUERY);

        const auto rx_payload = sendAndReceiveReply(&tx_payload);
        if (rx_payload.isError()) {
            return Result<uint8_t>::error(rx_payload.error());
        }
        const uint8_t power = get8BitFromNibbles(rx_payload.value(), 0);
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