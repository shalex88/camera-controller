#include "ViscaProtocol.h"

#include <algorithm>

#include "infrastructure/camera/transport/ITransport.h"

namespace {
    constexpr uint32_t VISCA_INPUT_BUFFER_SIZE = 16;
    constexpr uint32_t VISCA_PAYLOAD_SIZE = 14;
    constexpr uint32_t VISCA_SOCKET_NUM = 0;
    constexpr uint8_t VISCA_START_BYTE = 0x80;
    constexpr uint8_t VISCA_RESPONSE_START_BYTE = 0x90;
    constexpr uint8_t VISCA_BROADCAST_RESPONSE_BYTE = 0x88;
    constexpr uint8_t VISCA_COMMAND = 0x01;
    constexpr uint8_t VISCA_INQUIRY = 0x09;
    constexpr uint8_t VISCA_TERMINATOR = 0xFF;
    constexpr uint8_t VISCA_CATEGORY_INTERFACE = 0x00;
    constexpr uint8_t VISCA_CATEGORY_CAMERA1 = 0x04;
    constexpr uint8_t VISCA_CATEGORY_PAN_TILTER = 0x06;
    constexpr uint8_t VISCA_CATEGORY_CAMERA2 = 0x07;
    // Commands/inquiries codes
    constexpr uint8_t VISCA_POWER = 0x00;
    constexpr uint8_t VISCA_ADDRESS = 0x30;
    constexpr uint8_t VISCA_DEVICE_INFO = 0x02;
    constexpr uint8_t VISCA_KEYLOCK = 0x17;
    constexpr uint8_t VISCA_ID = 0x22;
    constexpr uint8_t VISCA_ZOOM = 0x07;
    constexpr uint8_t VISCA_ZOOM_STOP = 0x00;
    constexpr uint8_t VISCA_ZOOM_TELE = 0x02;
    constexpr uint8_t VISCA_ZOOM_WIDE = 0x03;
    constexpr uint8_t VISCA_ZOOM_TELE_SPEED = 0x20;
    constexpr uint8_t VISCA_ZOOM_WIDE_SPEED = 0x30;
    constexpr uint8_t VISCA_ZOOM_VALUE = 0x47;
    constexpr uint8_t VISCA_ZOOM_FOCUS_VALUE = 0x47;
    constexpr uint8_t VISCA_DZOOM = 0x06;
    constexpr uint8_t VISCA_DZOOM_VALUE = 0x46;
    constexpr uint8_t VISCA_DZOOM_LIMIT = 0x26; /* implemented for H10 */
    constexpr uint8_t VISCA_DZOOM_1X = 0x00;
    constexpr uint8_t VISCA_DZOOM_1_5X = 0x01;
    constexpr uint8_t VISCA_DZOOM_2X = 0x02;
    constexpr uint8_t VISCA_DZOOM_4X = 0x03;
    constexpr uint8_t VISCA_DZOOM_8X = 0x04;
    constexpr uint8_t VISCA_DZOOM_12X = 0x05;
    constexpr uint8_t VISCA_DZOOM_MODE = 0x36;
    constexpr uint8_t VISCA_DZOOM_COMBINE = 0x00;
    constexpr uint8_t VISCA_DZOOM_SEPARATE = 0x01;
    constexpr uint8_t VISCA_FOCUS = 0x08;
    constexpr uint8_t VISCA_FOCUS_STOP = 0x00;
    constexpr uint8_t VISCA_FOCUS_FAR = 0x02;
    constexpr uint8_t VISCA_FOCUS_NEAR = 0x03;
    constexpr uint8_t VISCA_FOCUS_FAR_SPEED = 0x20;
    constexpr uint8_t VISCA_FOCUS_NEAR_SPEED = 0x30;
    constexpr uint8_t VISCA_FOCUS_VALUE = 0x48;
    constexpr uint8_t VISCA_FOCUS_AUTO = 0x38;
    constexpr uint8_t VISCA_FOCUS_AUTO_MAN = 0x10;
    constexpr uint8_t VISCA_FOCUS_ONE_PUSH = 0x18;
    constexpr uint8_t VISCA_FOCUS_ONE_PUSH_TRIG = 0x01;
    constexpr uint8_t VISCA_FOCUS_ONE_PUSH_INF = 0x02;
    constexpr uint8_t VISCA_FOCUS_AUTO_SENSE = 0x58;
    constexpr uint8_t VISCA_FOCUS_AUTO_SENSE_HIGH = 0x02;
    constexpr uint8_t VISCA_FOCUS_AUTO_SENSE_LOW = 0x03;
    constexpr uint8_t VISCA_FOCUS_NEAR_LIMIT = 0x28;
    constexpr uint8_t VISCA_WB = 0x35;
    constexpr uint8_t VISCA_WB_AUTO = 0x00;
    constexpr uint8_t VISCA_WB_INDOOR = 0x01;
    constexpr uint8_t VISCA_WB_OUTDOOR = 0x02;
    constexpr uint8_t VISCA_WB_ONE_PUSH = 0x03;
    constexpr uint8_t VISCA_WB_ATW = 0x04;
    constexpr uint8_t VISCA_WB_MANUAL = 0x05;
    constexpr uint8_t VISCA_WB_TRIGGER = 0x10;
    constexpr uint8_t VISCA_WB_ONE_PUSH_TRIG = 0x05;
    constexpr uint8_t VISCA_RGAIN = 0x03;
    constexpr uint8_t VISCA_RGAIN_VALUE = 0x43;
    constexpr uint8_t VISCA_BGAIN = 0x04;
    constexpr uint8_t VISCA_BGAIN_VALUE = 0x44;
    constexpr uint8_t VISCA_AUTO_EXP = 0x39;
    constexpr uint8_t VISCA_AUTO_EXP_FULL_AUTO = 0x00;
    constexpr uint8_t VISCA_AUTO_EXP_MANUAL = 0x03;
    constexpr uint8_t VISCA_AUTO_EXP_SHUTTER_PRIORITY = 0x0A;
    constexpr uint8_t VISCA_AUTO_EXP_IRIS_PRIORITY = 0x0B;
    constexpr uint8_t VISCA_AUTO_EXP_GAIN_PRIORITY = 0x0C;
    constexpr uint8_t VISCA_AUTO_EXP_BRIGHT = 0x0D;
    constexpr uint8_t VISCA_AUTO_EXP_SHUTTER_AUTO = 0x1A;
    constexpr uint8_t VISCA_AUTO_EXP_IRIS_AUTO = 0x1B;
    constexpr uint8_t VISCA_AUTO_EXP_GAIN_AUTO = 0x1C;
    constexpr uint8_t VISCA_SLOW_SHUTTER = 0x5A;
    constexpr uint8_t VISCA_SLOW_SHUTTER_AUTO = 0x02;
    constexpr uint8_t VISCA_SLOW_SHUTTER_MANUAL = 0x03;
    constexpr uint8_t VISCA_SHUTTER = 0x0A;
    constexpr uint8_t VISCA_SHUTTER_VALUE = 0x4A;
    constexpr uint8_t VISCA_IRIS = 0x0B;
    constexpr uint8_t VISCA_IRIS_VALUE = 0x4B;
    constexpr uint8_t VISCA_GAIN = 0x0C;
    constexpr uint8_t VISCA_GAIN_VALUE = 0x4C;
    constexpr uint8_t VISCA_BRIGHT = 0x0D;
    constexpr uint8_t VISCA_BRIGHT_VALUE = 0x4D;
    constexpr uint8_t VISCA_EXP_COMP = 0x0E;
    constexpr uint8_t VISCA_EXP_COMP_POWER = 0x3E;
    constexpr uint8_t VISCA_EXP_COMP_VALUE = 0x4E;
    constexpr uint8_t VISCA_BACKLIGHT_COMP = 0x33;
    constexpr uint8_t VISCA_SPOT_AE = 0x59;
    constexpr uint8_t VISCA_SPOT_AE_POSITION = 0x29;
    constexpr uint8_t VISCA_APERTURE = 0x02;
    constexpr uint8_t VISCA_APERTURE_VALUE = 0x42;
    constexpr uint8_t VISCA_ZERO_LUX = 0x01;
    constexpr uint8_t VISCA_IR_LED = 0x31;
    constexpr uint8_t VISCA_WIDE_MODE = 0x60;
    constexpr uint8_t VISCA_WIDE_MODE_OFF = 0x00;
    constexpr uint8_t VISCA_WIDE_MODE_CINEMA = 0x01;
    constexpr uint8_t VISCA_WIDE_MODE_16_9 = 0x02;
    constexpr uint8_t VISCA_MIRROR = 0x61;
    constexpr uint8_t VISCA_FREEZE = 0x62;
    constexpr uint8_t VISCA_PICTURE_EFFECT = 0x63;
    constexpr uint8_t VISCA_PICTURE_EFFECT_OFF = 0x00;
    constexpr uint8_t VISCA_PICTURE_EFFECT_PASTEL = 0x01;
    constexpr uint8_t VISCA_PICTURE_EFFECT_NEGATIVE = 0x02;
    constexpr uint8_t VISCA_PICTURE_EFFECT_SEPIA = 0x03;
    constexpr uint8_t VISCA_PICTURE_EFFECT_BW = 0x04;
    constexpr uint8_t VISCA_PICTURE_EFFECT_SOLARIZE = 0x05;
    constexpr uint8_t VISCA_PICTURE_EFFECT_MOSAIC = 0x06;
    constexpr uint8_t VISCA_PICTURE_EFFECT_SLIM = 0x07;
    constexpr uint8_t VISCA_PICTURE_EFFECT_STRETCH = 0x08;
    constexpr uint8_t VISCA_DIGITAL_EFFECT = 0x64;
    constexpr uint8_t VISCA_DIGITAL_EFFECT_OFF = 0x00;
    constexpr uint8_t VISCA_DIGITAL_EFFECT_STILL = 0x01;
    constexpr uint8_t VISCA_DIGITAL_EFFECT_FLASH = 0x02;
    constexpr uint8_t VISCA_DIGITAL_EFFECT_LUMI = 0x03;
    constexpr uint8_t VISCA_DIGITAL_EFFECT_TRAIL = 0x04;
    constexpr uint8_t VISCA_DIGITAL_EFFECT_LEVEL = 0x65;
    constexpr uint8_t VISCA_CAM_STABILIZER = 0x34;
    constexpr uint8_t VISCA_MEMORY = 0x3F;
    constexpr uint8_t VISCA_MEMORY_RESET = 0x00;
    constexpr uint8_t VISCA_MEMORY_SET = 0x01;
    constexpr uint8_t VISCA_MEMORY_RECALL = 0x02;
    constexpr uint8_t VISCA_MEMORY_0 = 0x00;
    constexpr uint8_t VISCA_MEMORY_1 = 0x01;
    constexpr uint8_t VISCA_MEMORY_2 = 0x02;
    constexpr uint8_t VISCA_MEMORY_3 = 0x03;
    constexpr uint8_t VISCA_MEMORY_4 = 0x04;
    constexpr uint8_t VISCA_MEMORY_5 = 0x05;
    constexpr uint8_t VISCA_MEMORY_CUSTOM = 0x7F;
    constexpr uint8_t VISCA_DISPLAY = 0x15;
    constexpr uint8_t VISCA_DISPLAY_TOGGLE = 0x10;
    constexpr uint8_t VISCA_DATE_TIME_SET = 0x70;
    constexpr uint8_t VISCA_DATE_DISPLAY = 0x71;
    constexpr uint8_t VISCA_TIME_DISPLAY = 0x72;
    constexpr uint8_t VISCA_TITLE_DISPLAY = 0x74;
    constexpr uint8_t VISCA_TITLE_DISPLAY_CLEAR = 0x00;
    constexpr uint8_t VISCA_TITLE_SET = 0x73;
    constexpr uint8_t VISCA_TITLE_SET_PARAMS = 0x00;
    constexpr uint8_t VISCA_TITLE_SET_PART1 = 0x01;
    constexpr uint8_t VISCA_TITLE_SET_PART2 = 0x02;
    constexpr uint8_t VISCA_IRRECEIVE = 0x08;
    constexpr uint8_t VISCA_IRRECEIVE_ONOFF = 0x10;
    constexpr uint8_t VISCA_PT_DRIVE = 0x01;
    constexpr uint8_t VISCA_PT_DRIVE_HORIZ_LEFT = 0x01;
    constexpr uint8_t VISCA_PT_DRIVE_HORIZ_RIGHT = 0x02;
    constexpr uint8_t VISCA_PT_DRIVE_HORIZ_STOP = 0x03;
    constexpr uint8_t VISCA_PT_DRIVE_VERT_UP = 0x01;
    constexpr uint8_t VISCA_PT_DRIVE_VERT_DOWN = 0x02;
    constexpr uint8_t VISCA_PT_DRIVE_VERT_STOP = 0x03;
    constexpr uint8_t VISCA_PT_ABSOLUTE_POSITION = 0x02;
    constexpr uint8_t VISCA_PT_RELATIVE_POSITION = 0x03;
    constexpr uint8_t VISCA_PT_HOME = 0x04;
    constexpr uint8_t VISCA_PT_RESET = 0x05;
    constexpr uint8_t VISCA_PT_LIMITSET = 0x07;
    constexpr uint8_t VISCA_PT_LIMITSET_SET = 0x00;
    constexpr uint8_t VISCA_PT_LIMITSET_CLEAR = 0x01;
    constexpr uint8_t VISCA_PT_LIMITSET_SET_UR = 0x01;
    constexpr uint8_t VISCA_PT_LIMITSET_SET_DL = 0x00;
    constexpr uint8_t VISCA_PT_DATASCREEN = 0x06;
    constexpr uint8_t VISCA_PT_DATASCREEN_ONOFF = 0x10;
    constexpr uint8_t VISCA_PT_VIDEOSYSTEM_INQ = 0x23;
    constexpr uint8_t VISCA_PT_MODE_INQ = 0x10;
    constexpr uint8_t VISCA_PT_MAXSPEED_INQ = 0x11;
    constexpr uint8_t VISCA_PT_POSITION_INQ = 0x12;
    constexpr uint8_t VISCA_PT_DATASCREEN_INQ = 0x06;
    /**************************/
    /* DIRECT REGISTER ACCESS */
    /**************************/
    constexpr uint8_t VISCA_REGISTER_VALUE = 0x24;
    constexpr uint8_t VISCA_REGISTER_VISCA_BAUD = 0x00;
    constexpr uint8_t VISCA_REGISTER_BD9600 = 0x00;
    constexpr uint8_t VISCA_REGISTER_BD19200 = 0x01;
    constexpr uint8_t VISCA_REGISTER_BD38400 = 0x02;
    /* FCB-H10: Video Standard */
    constexpr uint8_t VISCA_REGISTER_VIDEO_SIGNAL = 0x70;
    constexpr uint8_t VISCA_REGISTER_VIDEO_1080I_60 = 0x01;
    constexpr uint8_t VISCA_REGISTER_VIDEO_720P_60 = 0x02;
    constexpr uint8_t VISCA_REGISTER_VIDEO_D1_CROP_60 = 0x03;
    constexpr uint8_t VISCA_REGISTER_VIDEO_D1_SQ_60 = 0x04;
    constexpr uint8_t VISCA_REGISTER_VIDEO_1080I_50 = 0x11;
    constexpr uint8_t VISCA_REGISTER_VIDEO_720P_50 = 0x12;
    constexpr uint8_t VISCA_REGISTER_VIDEO_D1_CROP_50 = 0x13;
    constexpr uint8_t VISCA_REGISTER_VIDEO_D1_SQ_50 = 0x14;
    /*****************/
    /* D30/D31 CODES */
    /*****************/
    constexpr uint8_t VISCA_WIDE_CON_LENS = 0x26;
    constexpr uint8_t VISCA_WIDE_CON_LENS_SET = 0x00;
    constexpr uint8_t VISCA_AT_MODE = 0x01;
    constexpr uint8_t VISCA_AT_ONOFF = 0x10;
    constexpr uint8_t VISCA_AT_AE = 0x02;
    constexpr uint8_t VISCA_AT_AUTOZOOM = 0x03;
    constexpr uint8_t VISCA_ATMD_FRAMEDISPLAY = 0x04;
    constexpr uint8_t VISCA_AT_FRAMEOFFSET = 0x05;
    constexpr uint8_t VISCA_ATMD_STARTSTOP = 0x06;
    constexpr uint8_t VISCA_AT_CHASE = 0x07;
    constexpr uint8_t VISCA_AT_CHASE_NEXT = 0x10;
    constexpr uint8_t VISCA_MD_MODE = 0x08;
    constexpr uint8_t VISCA_MD_ONOFF = 0x10;
    constexpr uint8_t VISCA_MD_FRAME = 0x09;
    constexpr uint8_t VISCA_MD_DETECT = 0x0A;
    constexpr uint8_t VISCA_MD_ADJUST = 0x00;
    constexpr uint8_t VISCA_MD_ADJUST_YLEVEL = 0x0B;
    constexpr uint8_t VISCA_MD_ADJUST_HUELEVEL = 0x0C;
    constexpr uint8_t VISCA_MD_ADJUST_SIZE = 0x0D;
    constexpr uint8_t VISCA_MD_ADJUST_DISPTIME = 0x0F;
    constexpr uint8_t VISCA_MD_ADJUST_REFTIME = 0x0B;
    constexpr uint8_t VISCA_MD_ADJUST_REFMODE = 0x10;
    constexpr uint8_t VISCA_AT_ENTRY = 0x15;
    constexpr uint8_t VISCA_AT_LOSTINFO = 0x20;
    constexpr uint8_t VISCA_MD_LOSTINFO = 0x21;
    constexpr uint8_t VISCA_ATMD_LOSTINFO1 = 0x20;
    constexpr uint8_t VISCA_ATMD_LOSTINFO2 = 0x07;
    constexpr uint8_t VISCA_MD_MEASURE_MODE_1 = 0x27;
    constexpr uint8_t VISCA_MD_MEASURE_MODE_2 = 0x28;
    constexpr uint8_t VISCA_ATMD_MODE = 0x22;
    constexpr uint8_t VISCA_AT_MODE_QUERY = 0x23; // CAM_MemSave
    constexpr uint8_t VISCA_MD_MODE_QUERY = 0x24;
    constexpr uint8_t VISCA_MD_REFTIME_QUERY = 0x11;
    constexpr uint8_t VISCA_AT_POSITION = 0x20;
    constexpr uint8_t VISCA_MD_POSITION = 0x21;
    /* Generic definitions */
    constexpr uint8_t VISCA_ON = 0x02;
    constexpr uint8_t VISCA_OFF = 0x03;
    constexpr uint8_t VISCA_RESET = 0x00;
    constexpr uint8_t VISCA_UP = 0x02;
    constexpr uint8_t VISCA_DOWN = 0x03;

    enum class ResponseType : uint32_t {
        Clear = 0x40,
        Address = 0x30,
        Ack = 0x40,
        Completed = 0x50,
        Error = 0x60
    };

    enum class ResultCode : uint8_t {
        Success = 0x00,
        Failure = 0xFF,
        ErrorMessageLength = 0x01,
        ErrorSyntax = 0x02,
        ErrorCmdBufferFull = 0x03,
        ErrorCmdCancelled = 0x04,
        ErrorNoSocket = 0x05,
        ErrorCmdNotExecutable = 0x41
    };

    enum class CameraVendors : uint16_t {
        Sony = 0x0020
    };

    enum class CameraModels : uint16_t {
        EW9500H = 0x070F
    };
}

namespace camera_service::infrastructure {
    struct ViscaProtocol::ViscaPayload {
        std::array<uint8_t, VISCA_PAYLOAD_SIZE> data{};
        size_t size = 0;
    };

    std::string_view getViscaErrorMessage(const ResultCode error_code) {
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
            default: {
                thread_local std::array<char, 64> buffer{};
                const auto [out, size] = std::format_to_n(buffer.begin(), buffer.size() - 1,
                                                          "Unknown error: 0x{:02X}",
                                                          static_cast<uint32_t>(error_code));
                *out = '\0';
                return std::string_view{buffer.data(), static_cast<std::size_t>(out - buffer.begin())};
            }
        }
    }

    std::string_view getCameraVendor(const uint16_t vendor) {
        switch (static_cast<CameraVendors>(vendor)) {
            case CameraVendors::Sony:
                return "Sony";
            default:
                return "Unknown";
        }
    }

    std::string_view getCameraModel(const uint16_t model) {
        switch (static_cast<CameraModels>(model)) {
            case CameraModels::EW9500H:
                return "EW9500H";
            default:
                return "Unknown";
        }
    }

    std::vector<uint8_t> decode(const std::span<const uint8_t> buffer) {
        if (buffer.size() < 2) {
            return {};
        }

        if (buffer.front() != VISCA_RESPONSE_START_BYTE && buffer.front() != VISCA_BROADCAST_RESPONSE_BYTE) {
            return {};
        }

        const auto terminator_it = std::ranges::find(buffer, static_cast<uint8_t>(VISCA_TERMINATOR));
        if (terminator_it == buffer.end()) {
            return {};
        }

        const auto buffer_size = std::distance(buffer.begin(), terminator_it) + 1;
        const auto payload_size = buffer_size - 3;

        if (payload_size <= 0) {
            return {};
        }

        std::vector<uint8_t> payload;
        payload.reserve(payload_size);
        payload.assign(buffer.begin() + 2, buffer.begin() + buffer_size - 1);

        return payload;
    }

    void appendByte(ViscaProtocol::ViscaPayload* payload, const uint8_t byte) {
        payload->data[payload->size++] = byte;
    }

    void appendAsNibbles(ViscaProtocol::ViscaPayload* payload, const uint16_t value) {
        appendByte(payload, (value & 0xF000) >> 12);
        appendByte(payload, (value & 0x0F00) >> 8);
        appendByte(payload, (value & 0x00F0) >> 4);
        appendByte(payload, (value & 0x000F));
    }

    uint16_t get16BitFromNibbles(const ViscaProtocol::ViscaPayload& payload, const size_t index) {
        const auto b0 = static_cast<uint16_t>(payload.data[index]) << 12;
        const auto b1 = static_cast<uint16_t>(payload.data[index + 1]) << 8;
        const auto b2 = static_cast<uint16_t>(payload.data[index + 2]) << 4;
        const auto b3 = static_cast<uint16_t>(payload.data[index + 3]);
        return static_cast<uint16_t>(b0 | b1 | b2 | b3);
    }

    uint8_t get8Bit(const ViscaProtocol::ViscaPayload& payload, const size_t index) {
        return payload.data[index];
    }

    uint8_t get8BitFromNibbles(const ViscaProtocol::ViscaPayload& payload, const size_t index) {
        const auto high = static_cast<uint16_t>(payload.data[index]) << 4;
        const auto low = static_cast<uint16_t>(payload.data[index + 1]);
        return static_cast<uint8_t>(high | low);
    }

    uint16_t get16Bit(const ViscaProtocol::ViscaPayload& payload, const size_t index) {
        const auto high = static_cast<uint16_t>(payload.data[index]) << 8;
        const auto low = static_cast<uint16_t>(payload.data[index + 1]);
        return static_cast<uint16_t>(high | low);
    }

    std::span<uint8_t> serialize(ViscaProtocol::ViscaPayload* payload) {
        return {payload->data.data(), payload->size};
    }

    ViscaProtocol::ViscaPayload deserialize(std::span<const uint8_t> buffer) {
        ViscaProtocol::ViscaPayload payload{};
        payload.size = buffer.size();
        std::ranges::copy(buffer, payload.data.begin());
        return payload;
    }

    ViscaProtocol::ViscaProtocol(std::unique_ptr<ITransport> transport) : transport_(std::move(transport)) {
        LOG_TRACE("Visca constructor called");
    }

    ViscaProtocol::~ViscaProtocol() {
        LOG_TRACE("Visca destructor called");
    }

    Result<void> ViscaProtocol::setAddress() {
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

    Result<void> ViscaProtocol::clear() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, 0x00);
        appendByte(&tx_payload, 0x01);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }
        return Result<void>::success();
    }

    Result<std::string_view> ViscaProtocol::getCameraInfo() const {
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

    Result<void> ViscaProtocol::open() const {
        return transport_->open();
    }

    Result<void> ViscaProtocol::close() const {
        return transport_->close();
    }

    Result<void> ViscaProtocol::setPower(const uint8_t power) const {
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

    Result<void> ViscaProtocol::setKeylock(const uint8_t power) const {
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

    Result<void> ViscaProtocol::setCameraId(const uint16_t id) const {
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

    Result<void> ViscaProtocol::setZoomTele() const {
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

    Result<void> ViscaProtocol::setZoomWide() const {
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

    Result<void> ViscaProtocol::setZoomStop() const {
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

    Result<void> ViscaProtocol::setZoomTeleSpeed(const uint32_t speed) const {
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

    Result<void> ViscaProtocol::setZoomWideSpeed(const uint32_t speed) const {
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

    Result<void> ViscaProtocol::setZoomValue(const uint16_t zoom) const {
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

    Result<void> ViscaProtocol::setZoomAndFocusValue(const uint16_t zoom, const uint16_t focus) const {
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

    Result<void> ViscaProtocol::setDzoomValue(const uint8_t value) const {
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

    Result<void> ViscaProtocol::setDzoomLimit(const uint8_t limit) const {
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

    Result<void> ViscaProtocol::setDzoomMode(const uint8_t power) const {
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

    Result<void> ViscaProtocol::setFocusFar() const {
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

    Result<void> ViscaProtocol::setFocusNear() const {
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

    Result<void> ViscaProtocol::setFocusStop() const {
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

    Result<void> ViscaProtocol::setFocusFarSpeed(const uint32_t speed) const {
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

    Result<void> ViscaProtocol::setFocusNearSpeed(const uint32_t speed) const {
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

    Result<void> ViscaProtocol::setFocusValue(const uint16_t focus) const {
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

    Result<void> ViscaProtocol::setFocusAuto(const bool on) const {
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

    Result<void> ViscaProtocol::setFocusOnePush() const {
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

    Result<void> ViscaProtocol::setFocusInfinity() const {
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

    Result<void> ViscaProtocol::setFocusAutosenseHigh() const {
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

    Result<void> ViscaProtocol::setFocusAutosenseLow() const {
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

    Result<void> ViscaProtocol::setFocusNearLimit(const uint16_t limit) const {
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

    Result<void> ViscaProtocol::setWhitebalMode(const uint8_t mode) const {
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

    Result<void> ViscaProtocol::setWhitebalOnePush() const {
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

    Result<void> ViscaProtocol::setRgainUp() const {
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

    Result<void> ViscaProtocol::setRgainDown() const {
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

    Result<void> ViscaProtocol::setRgainReset() const {
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

    Result<void> ViscaProtocol::setRgainValue(const uint8_t value) const {
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

    Result<void> ViscaProtocol::setBgainUp() const {
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

    Result<void> ViscaProtocol::setBgainDown() const {
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

    Result<void> ViscaProtocol::setBgainReset() const {
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

    Result<void> ViscaProtocol::setBgainValue(const uint8_t value) const {
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

    Result<void> ViscaProtocol::setShutterUp() const {
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

    Result<void> ViscaProtocol::setShutterDown() const {
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

    Result<void> ViscaProtocol::setShutterReset() const {
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

    Result<void> ViscaProtocol::setShutterValue(const uint8_t value) const {
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

    Result<void> ViscaProtocol::setIrisUp() const {
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

    Result<void> ViscaProtocol::setIrisDown() const {
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

    Result<void> ViscaProtocol::setIrisReset() const {
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

    Result<void> ViscaProtocol::setIrisValue(const uint8_t value) const {
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

    Result<void> ViscaProtocol::setGainUp() const {
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

    Result<void> ViscaProtocol::setGainDown() const {
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

    Result<void> ViscaProtocol::setGainReset() const {
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

    Result<void> ViscaProtocol::setGainValue(const uint8_t value) const {
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

    Result<void> ViscaProtocol::setBrightUp() const {
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

    Result<void> ViscaProtocol::setBrightDown() const {
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

    Result<void> ViscaProtocol::setBrightReset() const {
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

    Result<void> ViscaProtocol::setBrightValue(const uint16_t value) const {
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

    Result<void> ViscaProtocol::setApertureUp() const {
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

    Result<void> ViscaProtocol::setApertureDown() const {
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

    Result<void> ViscaProtocol::setApertureReset() const {
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

    Result<void> ViscaProtocol::setApertureValue(const uint8_t value) const {
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

    Result<void> ViscaProtocol::setExpCompUp() const {
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

    Result<void> ViscaProtocol::setExpCompDown() const {
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

    Result<void> ViscaProtocol::setExpCompReset() const {
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

    Result<void> ViscaProtocol::setExpCompValue(const uint8_t value) const {
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

    Result<void> ViscaProtocol::setExpCompPower(const uint8_t power) const {
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

    Result<void> ViscaProtocol::setAutoExpMode(const uint8_t mode) const {
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

    Result<void> ViscaProtocol::setSlowShutterAuto(const uint8_t power) const {
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

    Result<void> ViscaProtocol::setBacklightComp(const bool on) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_BACKLIGHT_COMP);
        if (on) {
            appendByte(&tx_payload, VISCA_ON);
        }
        else {
            appendByte(&tx_payload, VISCA_OFF);
        }

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> ViscaProtocol::setZeroLuxShot(const uint8_t power) const {
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

    Result<void> ViscaProtocol::setIrLed(const uint8_t power) const {
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

    Result<void> ViscaProtocol::setWideMode(const uint8_t mode) const {
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

    Result<void> ViscaProtocol::setMirror(const uint8_t power) const {
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

    Result<void> ViscaProtocol::setFreeze(const uint8_t power) const {
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

    Result<void> ViscaProtocol::setPictureEffect(const uint8_t mode) const {
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

    Result<void> ViscaProtocol::setDigitalEffect(const uint8_t mode) const {
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

    Result<void> ViscaProtocol::setDigitalEffectLevel(const uint8_t level) const {
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

    Result<void> ViscaProtocol::setCamStabilizer(const bool power) const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA1);
        appendByte(&tx_payload, VISCA_CAM_STABILIZER);

        if (power) {
            appendByte(&tx_payload, VISCA_ON);
        }
        else {
            appendByte(&tx_payload, VISCA_OFF);
        }

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> ViscaProtocol::memorySet(const uint8_t channel) const {
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

    Result<void> ViscaProtocol::memoryRecall(const uint8_t channel) const {
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

    Result<void> ViscaProtocol::memoryReset(const uint8_t channel) const {
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

    Result<void> ViscaProtocol::setDisplay(const uint8_t power) const {
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

    Result<void> ViscaProtocol::setDateTime(const uint16_t year, const uint16_t month, const uint16_t day, const uint16_t hour,
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

    Result<void> ViscaProtocol::setDateDisplay(const uint8_t power) const {
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

    Result<void> ViscaProtocol::setTimeDisplay(const uint8_t power) const {
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

    Result<void> ViscaProtocol::setTitleDisplay(const uint8_t power) const {
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

    Result<void> ViscaProtocol::setTitleClear() const {
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

    Result<void> ViscaProtocol::setTitleParams(const ViscaTitleData* title) const {
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

    Result<void> ViscaProtocol::setTitle(const ViscaTitleData* title) const {
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

    Result<void> ViscaProtocol::setIrreceiveOn() const {
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

    Result<void> ViscaProtocol::setIrreceiveOff() const {
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

    Result<void> ViscaProtocol::setIrreceiveOnoff() const {
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

    Result<void> ViscaProtocol::setPanTiltUp(const uint8_t pan_speed, const uint8_t tilt_speed) const {
        if (pan_speed < 1 || pan_speed > 18) {
            return Result<void>::error("Pan speed should be in the range 01 - 18");
        }
        if (tilt_speed < 1 || tilt_speed > 14) {
            return Result<void>::error("Tilt speed should be in the range 01 - 18");
        }

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

    Result<void> ViscaProtocol::setPanTiltDown(const uint8_t pan_speed, const uint8_t tilt_speed) const {
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

    Result<void> ViscaProtocol::setPanTiltLeft(const uint8_t pan_speed, const uint8_t tilt_speed) const {
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

    Result<void> ViscaProtocol::setPanTiltRight(const uint8_t pan_speed, const uint8_t tilt_speed) const {
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

    Result<void> ViscaProtocol::setPanTiltUpleft(const uint8_t pan_speed, const uint8_t tilt_speed) const {
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

    Result<void> ViscaProtocol::setPanTiltUpright(const uint8_t pan_speed, const uint8_t tilt_speed) const {
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

    Result<void> ViscaProtocol::setPanTiltDownleft(const uint8_t pan_speed, const uint8_t tilt_speed) const {
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

    Result<void> ViscaProtocol::setPanTiltDownright(const uint8_t pan_speed, const uint8_t tilt_speed) const {
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

    Result<void> ViscaProtocol::setPanTiltStop(const uint8_t pan_speed, const uint8_t tilt_speed) const {
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

    Result<void> ViscaProtocol::setPanTiltAbsolutePosition(const uint8_t pan_speed, const uint8_t tilt_speed,
                                                   const uint16_t pan_position, const uint16_t tilt_position) const {
        if (pan_speed < 1 || pan_speed > 18) {
            return Result<void>::error("Pan speed should be in the range 01 - 18");
        }
        if (tilt_speed < 1 || tilt_speed > 14) {
            return Result<void>::error("Tilt speed should be in the range 01 - 14");
        }
        if (pan_position < 0xFC90 || pan_position > 0x0370) {
            return Result<void>::error("Pan position should be in the range -880 - 880");
        }
        if (tilt_position < 0xFED4 || tilt_position > 0x012C) {
            return Result<void>::error("Tilt position should be in the range -300 - 300");
        }

        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&tx_payload, VISCA_PT_ABSOLUTE_POSITION);
        appendByte(&tx_payload, pan_speed);
        appendByte(&tx_payload, tilt_speed);
        appendAsNibbles(&tx_payload, pan_position);
        appendAsNibbles(&tx_payload, tilt_position);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> ViscaProtocol::setPanTiltRelativePosition(const uint8_t pan_speed, const uint8_t tilt_speed,
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

    Result<void> ViscaProtocol::setPanTiltHome() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&tx_payload, VISCA_PT_HOME);
        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> ViscaProtocol::setPanTiltReset() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_PAN_TILTER);
        appendByte(&tx_payload, VISCA_PT_RESET);
        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> ViscaProtocol::setPanTiltLimitUpright(const uint16_t pan_limit, const uint16_t tilt_limit) const {
        if (pan_limit < 0xFC90 || pan_limit > 0x370) {
            return Result<void>::error("Pan limit should be in the range -880 - 880");
        }
        if (tilt_limit < 0xFED4 || tilt_limit > 0x12C) {
            return Result<void>::error("Tilt limit should be in the range -300 - 300");
        }

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

    Result<void> ViscaProtocol::setPanTiltLimitDownleft(const uint16_t pan_limit, const uint16_t tilt_limit) const {
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

    Result<void> ViscaProtocol::setPanTiltLimitDownleftClear() const {
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

    Result<void> ViscaProtocol::setPanTiltLimitUprightClear() const {
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

    Result<void> ViscaProtocol::setDatascreenOn() const {
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

    Result<void> ViscaProtocol::setDatascreenOff() const {
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

    Result<void> ViscaProtocol::setDatascreenOnoff() const {
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

    Result<void> ViscaProtocol::setSpotAeOn() const {
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

    Result<void> ViscaProtocol::setSpotAeOff() const {
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

    Result<void> ViscaProtocol::setSpotAePosition(const uint8_t x_position, const uint8_t y_position) const {
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

    Result<uint8_t> ViscaProtocol::getPower() const {
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

    Result<uint8_t> ViscaProtocol::getDzoomValue() const {
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

    Result<uint8_t> ViscaProtocol::getDzoomLimit() const {
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

    Result<uint16_t> ViscaProtocol::getZoomValue() const {
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

    Result<bool> ViscaProtocol::getFocusAuto() const {
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

    Result<uint16_t> ViscaProtocol::getFocusValue() const {
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

    Result<uint8_t> ViscaProtocol::getFocusAutoSense() const {
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

    Result<uint16_t> ViscaProtocol::getFocusNearLimit() const {
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

    Result<uint8_t> ViscaProtocol::getWhitebalMode() const {
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

    Result<uint8_t> ViscaProtocol::getRgainValue() const {
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

    Result<uint8_t> ViscaProtocol::getBgainValue() const {
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

    Result<uint8_t> ViscaProtocol::getAutoExpMode() const {
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

    Result<uint8_t> ViscaProtocol::getSlowShutterAuto() const {
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

    Result<uint8_t> ViscaProtocol::getShutterValue() const {
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

    Result<uint8_t> ViscaProtocol::getIrisValue() const {
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

    Result<uint8_t> ViscaProtocol::getGainValue() const {
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

    Result<uint16_t> ViscaProtocol::getBrightValue() const {
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

    Result<uint8_t> ViscaProtocol::getExpCompPower() const {
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

    Result<uint8_t> ViscaProtocol::getExpCompValue() const {
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

    Result<bool> ViscaProtocol::getBacklightComp() const {
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

    Result<uint8_t> ViscaProtocol::getApertureValue() const {
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

    Result<uint8_t> ViscaProtocol::getZeroLuxShot() const {
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

    Result<uint8_t> ViscaProtocol::getIrLed() const {
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

    Result<uint8_t> ViscaProtocol::getWideMode() const {
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

    Result<uint8_t> ViscaProtocol::getMirror() const {
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

    Result<uint8_t> ViscaProtocol::getFreeze() const {
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

    Result<uint8_t> ViscaProtocol::getPictureEffect() const {
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

    Result<uint8_t> ViscaProtocol::getDigitalEffect() const {
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

    Result<uint16_t> ViscaProtocol::getDigitalEffectLevel() const {
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

    Result<uint8_t> ViscaProtocol::getMemory() const {
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

    Result<uint8_t> ViscaProtocol::getDisplay() const {
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

    Result<uint16_t> ViscaProtocol::getId() const {
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

    Result<uint8_t> ViscaProtocol::getVideoSystem() const {
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

    Result<uint16_t> ViscaProtocol::getPanTiltMode() const {
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

    Result<std::pair<uint8_t, uint8_t>> ViscaProtocol::getPanTiltMaxspeed() const {
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

    Result<std::pair<uint16_t, uint16_t>> ViscaProtocol::getPanTiltPosition() const {
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

    Result<uint8_t> ViscaProtocol::getDatascreen() const {
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

    Result<void> ViscaProtocol::setRegister(const uint8_t reg_num, const uint8_t reg_val) const {
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

    Result<uint8_t> ViscaProtocol::getRegister(const uint8_t reg_num) const {
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

    Result<ViscaProtocol::ViscaPayload> ViscaProtocol::sendAndReceiveReply(ViscaPayload* payload) const {
        const auto serialized_payload = serialize(payload);
        const auto frame = encode(serialized_payload);

        if (const auto result = transport_->write(frame); result.isError()) {
            return Result<ViscaPayload>::error(result.error());
        }

        std::array<uint8_t, VISCA_INPUT_BUFFER_SIZE> rx_buffer{};

        if (const auto result = transport_->read(rx_buffer); result.isError()) {
            return Result<ViscaPayload>::error(result.error());
        }
        else if (result.value() < 3) {
            return Result<ViscaPayload>::error("Received response is too short");
        }
        auto type = static_cast<ResponseType>(rx_buffer.at(1) & 0xF0);

        while (type == ResponseType::Ack) {
            if (const auto result = transport_->read(rx_buffer); result.isError()) {
                return Result<ViscaPayload>::error(result.error());
            }
            else if (result.value() < 3) {
                return Result<ViscaPayload>::error("Received response is too short");
            }
            type = static_cast<ResponseType>(rx_buffer.at(1) & 0xF0); //TODO: payload
        }

        if (type == ResponseType::Error) {
            const auto error_msg = getViscaErrorMessage(static_cast<ResultCode>(rx_buffer.at(2)));
            return Result<ViscaPayload>::error(std::string(error_msg));
        }

        if (type != ResponseType::Completed && type != ResponseType::Address && type != ResponseType::Clear) {
            return Result<ViscaPayload>::error("Unexpected response type from camera");
        }

        const auto response_payload = deserialize(decode(rx_buffer));

        return Result<ViscaPayload>::success(response_payload);
    }

    std::vector<uint8_t> ViscaProtocol::encode(std::span<const uint8_t> payload) const {
        const size_t frame_size = payload.size() + 2;

        // Use thread_local buffer to avoid allocations for common case
        thread_local std::array<uint8_t, VISCA_PAYLOAD_SIZE + 2> buffer{};

        uint8_t* frame_data;
        std::vector<uint8_t> frame_vec;

        if (frame_size <= buffer.size()) {
            frame_data = buffer.data();
        } else {
            frame_vec.resize(frame_size);
            frame_data = frame_vec.data();
        }

        frame_data[0] = VISCA_START_BYTE;
        frame_data[0] |= (VISCA_SOCKET_NUM << 4); // Should it always be 0?
        if (broadcast_ > 0) {
            frame_data[0] |= (broadcast_ << 3);
            frame_data[0] &= 0xF8;
        }
        else {
            frame_data[0] |= cam_address_;
        }

        std::ranges::copy(payload, frame_data + 1);

        frame_data[frame_size - 1] = VISCA_TERMINATOR;

        if (frame_size <= buffer.size()) {
            return std::vector<uint8_t>(frame_data, frame_data + frame_size);
        }
        return frame_vec;
    }

    /********************************/
    /* SPECIAL FUNCTIONS FOR D30/31 */
    /********************************/

    Result<void> ViscaProtocol::setWideConLens(const uint8_t power) const {
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

    Result<void> ViscaProtocol::setAtModeOnoff() const {
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

    Result<void> ViscaProtocol::setAtMode(const uint8_t power) const {
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

    Result<void> ViscaProtocol::setAtAeOnoff() const {
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

    Result<void> ViscaProtocol::setAtAe(const uint8_t power) const {
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

    Result<void> ViscaProtocol::setAtAutozoomOnoff() const {
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

    Result<void> ViscaProtocol::setAtAutozoom(const uint8_t power) const {
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

    Result<void> ViscaProtocol::setAtmdFramedisplayOnoff() const {
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

    Result<void> ViscaProtocol::setAtmdFramedisplay(const uint8_t power) const {
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

    Result<void> ViscaProtocol::setAtFrameoffsetOnoff() const {
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

    Result<void> ViscaProtocol::setAtFrameoffset(const uint8_t power) const {
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

    Result<void> ViscaProtocol::setAtmdStartstop() const {
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

    Result<void> ViscaProtocol::setAtChase(const uint8_t power) const {
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

    Result<void> ViscaProtocol::setAtChaseNext() const {
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

    Result<void> ViscaProtocol::setMdModeOnoff() const {
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

    Result<void> ViscaProtocol::setMdMode(const uint8_t power) const {
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

    Result<void> ViscaProtocol::setMdFrame() const {
        ViscaPayload tx_payload{};

        appendByte(&tx_payload, VISCA_COMMAND);
        appendByte(&tx_payload, VISCA_CATEGORY_CAMERA2);
        appendByte(&tx_payload, VISCA_MD_FRAME);

        if (const auto rx_payload = sendAndReceiveReply(&tx_payload); rx_payload.isError()) {
            return Result<void>::error(rx_payload.error());
        }

        return Result<void>::success();
    }

    Result<void> ViscaProtocol::setMdDetect() const {
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

    Result<void> ViscaProtocol::setAtEntry(const uint8_t power) const {
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

    Result<void> ViscaProtocol::setAtLostinfo() const {
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

    Result<void> ViscaProtocol::setMdLostinfo() const {
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

    Result<void> ViscaProtocol::setMdAdjustYlevel(const uint8_t power) const {
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

    Result<void> ViscaProtocol::setMdAdjustHuelevel(const uint8_t power) const {
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

    Result<void> ViscaProtocol::setMdAdjustSize(const uint8_t power) const {
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

    Result<void> ViscaProtocol::setMdAdjustDisptime(const uint8_t power) const {
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

    Result<void> ViscaProtocol::setMdAdjustRefmode(const uint8_t power) const {
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

    Result<void> ViscaProtocol::setMdAdjustReftime(const uint8_t power) const {
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

    Result<void> ViscaProtocol::setMdMeasureMode1Onoff() const {
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

    Result<void> ViscaProtocol::setMdMeasureMode1(const uint8_t power) const {
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

    Result<void> ViscaProtocol::setMdMeasureMode2Onoff() const {
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

    Result<void> ViscaProtocol::setMdMeasureMode2(const uint8_t power) const {
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

    Result<uint8_t> ViscaProtocol::getKeylock() const {
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

    Result<uint8_t> ViscaProtocol::getWideConLens() const {
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

    Result<uint8_t> ViscaProtocol::getAtmdMode() const {
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

    Result<uint16_t> ViscaProtocol::getAtMode() const {
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

    Result<uint8_t> ViscaProtocol::getAtEntry() const {
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

    Result<uint16_t> ViscaProtocol::getMdMode() const {
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

    Result<uint8_t> ViscaProtocol::getMdYlevel() const {
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

    Result<uint8_t> ViscaProtocol::getMdHuelevel() const {
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

    Result<uint8_t> ViscaProtocol::getMdSize() const {
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

    Result<uint8_t> ViscaProtocol::getMdDisptime() const {
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

    Result<uint8_t> ViscaProtocol::getMdRefmode() const {
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

    Result<uint8_t> ViscaProtocol::getMdReftime() const {
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
}
