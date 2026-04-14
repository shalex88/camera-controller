#include "VideoChannel.h"

#include "infrastructure/camera/transport/mmio/RegisterImplUio.h"

namespace service::infrastructure {
    namespace {
        constexpr uint32_t ENABLE = 0x00000001;
        constexpr uint32_t DISABLE = 0x00000000;

        struct Register {
            uint32_t address;
            uint32_t value;
        };

        struct GlobalRegisters {
            std::string uio = "/dev/uio12";
            Register video_channel_enable{};
            Register video_channel_disable{};
            Register video_channel_format{};
            Register mipi_word_count{};
            Register live_video_width{};
            Register live_video_height{};
            Register live_video_fps{};
        };

        struct MuxRegisters {
            std::string uio;
            Register camera_output_enable{};
            Register camera_output_disable{};
            Register camera_input_enable{};
            Register camera_input_disable{};
        };

        struct MipiRegisters {
            std::string uio;
            Register mipi_rx_enable{};
            Register mipi_rx_disable{};
            Register mipi_line_length{};
            Register mipi_protocol_config{};
        };

        struct TestPatternRegisters {
            std::string uio;
            Register test_pattern_enable{};
            Register test_pattern_frame_width{};
            Register test_pattern_frame_height{};
            Register test_pattern_active_video_width{};
            Register test_pattern_frame_to_frame_time{};
            Register test_pattern_type{};
        };

        struct VideoChannelConfig {
            GlobalRegisters global{};
            MuxRegisters mux{};
            MipiRegisters mipi{};
            TestPatternRegisters test_pattern{};
        };

        std::array<VideoChannelConfig, 4> channel_configs{
            // TODO: Channel 0 ADIMEC
            VideoChannelConfig{},
            // Channel 1 Sony
            VideoChannelConfig{
                .global = GlobalRegisters{
                    .video_channel_enable = {0x800B'0034, ENABLE},
                    .video_channel_disable = {0x800B'0034, DISABLE},
                    .video_channel_format = {0x800B'0014, 0x0000'0001},
                    .mipi_word_count = {0x800B'0024, 0x0000'12C0},
                    .live_video_width = {0x800B'0074, 0x0000'0000},
                    .live_video_height = {0x800B'0078, 0x0000'0000},
                    .live_video_fps = {0x800B'007C, 0x0000'0000}
                },
                .mux = MuxRegisters{
                    .uio = "/dev/uio7",
                    .camera_output_enable = {0x8006'0068, ENABLE},
                    .camera_output_disable = {0x8006'0068, DISABLE},
                    .camera_input_enable = {0x8006'0048, ENABLE},
                    .camera_input_disable = {0x8006'0048, DISABLE}
                },
                .mipi = MipiRegisters{
                    .uio = "/dev/uio14",
                    .mipi_rx_enable = {0x800D'0000, ENABLE},
                    .mipi_rx_disable = {0x800D'0000, DISABLE},
                    .mipi_line_length = {0x800D'0040, 1080},
                    .mipi_protocol_config = {0x800D'0004, 0x0000'E01B}
                },
                .test_pattern = TestPatternRegisters{
                    .uio = "/dev/uio6",
                    .test_pattern_enable = {0x8005009C, ENABLE},
                    .test_pattern_frame_width = {0x80050070, 1920},
                    .test_pattern_frame_height = {0x80050074, 1080},
                    .test_pattern_active_video_width = {0x80050078, 0x00000468},
                    .test_pattern_frame_to_frame_time = {0x8005007C, 0x007A1200},
                    .test_pattern_type = {0x80050008, 0x0000000F},
                }
            },
            // Channel 2 MWIR
            VideoChannelConfig{
                .global = GlobalRegisters{
                    .video_channel_enable = {0x800B0038, ENABLE},
                    .video_channel_disable = {0x800B0038, DISABLE},
                    .video_channel_format = {0x800B0018, 0x00000001},
                    .mipi_word_count = {0x800B0028, 0x00000C80},
                    .live_video_width = {0x800B'00A4, 0x0000'0000},
                    .live_video_height = {0x800B'00A8, 0x0000'0000},
                    .live_video_fps = {0x800B'00AC, 0x0000'0000}
                },
                .mux = MuxRegisters{
                    .uio = "/dev/uio9",
                    .camera_output_enable = {0x80080068, ENABLE},
                    .camera_output_disable = {0x80080068, DISABLE},
                    .camera_input_enable = {0x80080048, ENABLE},
                    .camera_input_disable = {0x80080048, DISABLE}
                },
                .mipi = MipiRegisters{
                    .uio = "/dev/uio15",
                    .mipi_rx_enable = {0x800E0000, ENABLE},
                    .mipi_rx_disable = {0x800E0000, DISABLE},
                    .mipi_line_length = {0x800E0040, 2048},
                    .mipi_protocol_config = {0x800E0004, 0x0000E01B}
                },
                .test_pattern = TestPatternRegisters{
                    .uio = "/dev/uio8",
                    .test_pattern_enable = {0x8007009C, ENABLE},
                    .test_pattern_frame_width = {0x80070070, 2560},
                    .test_pattern_frame_height = {0x80070074, 2048},
                    .test_pattern_active_video_width = {0x80070078, 0x00000329},
                    .test_pattern_frame_to_frame_time = {0x8007007C, 0x007A1200},
                    .test_pattern_type = {0x80070008, 0x0000000F},
                }
            },
            // TODO: Channel 3
            VideoChannelConfig{}
        };

        Result<void> configureChannel(const uint32_t channel_num) {
            LOG_DEBUG("Configuring video channel {}...", channel_num);
            auto [global, mux, mipi, test_pattern] = channel_configs.at(channel_num - 1); //FIXME: Remove '-1' when channel 0 is configured

            {
                const auto global_reg = std::make_unique<RegisterImplUio>(global.uio);
                if (const auto result = global_reg->set(global.video_channel_disable.address, global.video_channel_disable.value); result.isError()) {
                    return Result<void>::error("Failed to disable video channel");
                }
            }
            LOG_DEBUG("FPGA video channel disabled");

            {
                const auto mipi_reg = std::make_unique<RegisterImplUio>(mipi.uio);
                if (const auto result = mipi_reg->set(mipi.mipi_rx_disable.address, mipi.mipi_rx_disable.value); result.isError()) {
                    return Result<void>::error("Failed to disable MIPI RX");
                }
            }
            LOG_DEBUG("FPGA disabled MIPI RX");

            {
                const auto test_pattern_reg = std::make_unique<RegisterImplUio>(test_pattern.uio);
                if (const auto result = test_pattern_reg->set(test_pattern.test_pattern_type.address, test_pattern.test_pattern_type.value); result.isError()) {
                    return Result<void>::error("Failed to disable test pattern generator");
                }
                if (const auto result = test_pattern_reg->set(test_pattern.test_pattern_frame_width.address, test_pattern.test_pattern_frame_width.value); result.isError()) {
                    return Result<void>::error("Failed to set test pattern frame height");
                }
                if (const auto result = test_pattern_reg->set(test_pattern.test_pattern_frame_height.address, test_pattern.test_pattern_frame_height.value); result.isError()) {
                    return Result<void>::error("Failed to set test pattern line length");
                }
                if (const auto result = test_pattern_reg->set(test_pattern.test_pattern_active_video_width.address, test_pattern.test_pattern_active_video_width.value); result.isError()) {
                    return Result<void>::error("Failed to set test pattern active video width");
                }
                if (const auto result = test_pattern_reg->set(test_pattern.test_pattern_frame_to_frame_time.address, test_pattern.test_pattern_frame_to_frame_time.value); result.isError()) {
                    return Result<void>::error("Failed to set frame to frame time");
                }
                if (const auto result = test_pattern_reg->set(test_pattern.test_pattern_enable.address, test_pattern.test_pattern_enable.value); result.isError()) {
                    return Result<void>::error("Failed to enable test pattern generator");
                }
            }
            LOG_DEBUG("FPGA test pattern generator configured and enabled");

            {
                const auto mux_reg = std::make_unique<RegisterImplUio>(mux.uio);
                if (const auto result = mux_reg->set(mux.camera_output_disable.address, mux.camera_output_disable.value); result.isError()) {
                    return Result<void>::error("Failed to disable camera video");
                }
                if (const auto result = mux_reg->set(mux.camera_input_enable.address, mux.camera_input_enable.value); result.isError()) {
                    return Result<void>::error("Failed to set camera video input");
                }
            }
            LOG_DEBUG("FPGA enable test pattern output");

            {
                const auto global_reg = std::make_unique<RegisterImplUio>(global.uio);
                if (const auto result = global_reg->set(global.video_channel_format.address, global.video_channel_format.value); result.isError()) {
                    return Result<void>::error("Failed to reset video channel format");
                }
                if (const auto result = global_reg->set(global.mipi_word_count.address, global.mipi_word_count.value); result.isError()) {
                    return Result<void>::error("Failed to set video channel mipi word count in line (bytes)");
                }
            }
            LOG_DEBUG("FPGA configure video channel");

            {
                const auto mipi_reg = std::make_unique<RegisterImplUio>(mipi.uio);
                if (const auto result = mipi_reg->set(mipi.mipi_line_length.address, mipi.mipi_line_length.value); result.isError()) {
                    return Result<void>::error("Failed to set MIPI RX line length");
                }
                if (const auto result = mipi_reg->set(mipi.mipi_protocol_config.address, mipi.mipi_protocol_config.value); result.isError()) {
                    return Result<void>::error("Failed to set MIPI protocol configuration");
                }
                if (const auto result = mipi_reg->set(mipi.mipi_rx_enable.address, mipi.mipi_rx_enable.value); result.isError()) {
                    return Result<void>::error("Failed to enable MIPI RX");
                }
            }
            LOG_DEBUG("FPGA configure and enable MIPI RX");

            {
                const auto global_reg = std::make_unique<RegisterImplUio>(global.uio);
                if (const auto result = global_reg->set(global.video_channel_enable.address, global.video_channel_enable.value); result.isError()) {
                    return Result<void>::error("Failed to enable video channel");
                }
            }
            LOG_DEBUG("FPGA enable video channel");

            {
                const auto mux_reg = std::make_unique<RegisterImplUio>(mux.uio);
                if (const auto result = mux_reg->set(mux.camera_output_enable.address, mux.camera_output_enable.value); result.isError()) {
                    return Result<void>::error("Failed to enable camera video");
                }
            }
            LOG_DEBUG("FPGA enable camera output");

            return Result<void>::success();
        }

        Result<void> validateLiveVideo(const uint32_t channel_num) {
            // Channel 0
            // 0x800b0058 *4 // width
            // 0x800b005c // height
            // 0x800b0060 // FPS

            // Channel 3
            // 0x800b00d4 *4 // 1 width
            // 0x800b00d8 // 1 length
            // 0x800b00dc // 3 FPS
            LOG_DEBUG("Validating live video on channel {}...", channel_num);
            auto [global, mux, mipi, test_pattern] = channel_configs.at(channel_num);

            const auto global_reg = std::make_unique<RegisterImplUio>(global.uio);
            const auto width_result = global_reg->get(global.live_video_width.address);
            if (width_result.isError() || (width_result.value() * 4 != test_pattern.test_pattern_frame_width.value)) {
                return Result<void>::error("Failed validate width resolution");
            }
            const auto height_result = global_reg->get(global.live_video_height.address);
            if (height_result.isError() || (height_result.value() != test_pattern.test_pattern_frame_height.value)) {
                return Result<void>::error("Failed to validate height resolution");
            }
            const auto fps_result = global_reg->get(global.live_video_fps.address);
            if (fps_result.isError() || fps_result.value() == 0) {
                return Result<void>::error("Failed to validate live FPS");
            }

            LOG_DEBUG("Live video is available {}x{} {}FPS", width_result.value() * 4, height_result.value(), fps_result.value());

            return Result<void>::success();
        }
    } // unnamed namespace

    Result<void> VideoChannel::initialize(const uint32_t channel_num) {
        if (const auto result = configureChannel(channel_num); result.isError()) {
            return Result<void>::error("Failed to configure video channel: " + result.error());
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(80));

        if (const auto result = validateLiveVideo(channel_num); result.isError()) {
            return Result<void>::error("Live video is not available: " + result.error());
        }

        return Result<void>::success();
    }
} // namespace service::infrastructure