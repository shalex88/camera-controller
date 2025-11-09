# GenTL Producer Library
project(FpgaCXPProducer VERSION 1.0.0)

# Collect source files
set(GENTL_SOURCES
    GenTL.cpp
    SystemModule.cpp
    DeviceModule.cpp
    StreamModule.cpp
    ../genicam/FpgaTransport.cpp
)

set(GENTL_HEADERS
    GenTL.h
    GenTLImpl.h
    ../genicam/FpgaTransport.h
)

# Create shared library (.cti extension for GenTL producers)
add_library(${PROJECT_NAME} SHARED ${GENTL_SOURCES} ${GENTL_HEADERS})

# Set output name to .cti (GenTL Producer extension)
set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "FpgaCXP"
    SUFFIX ".cti"
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
)

# Include directories
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/infrastructure/camera/protocol/genicam/include
)

# Determine GenICam library path
if (CMAKE_CROSSCOMPILING)
    set(GENICAM_LIB_DIR "${CMAKE_SOURCE_DIR}/src/infrastructure/camera/protocol/genicam/lib/Linux64_ARM")
else()
    set(GENICAM_LIB_DIR "${CMAKE_SOURCE_DIR}/src/infrastructure/camera/protocol/genicam/lib/Linux64_x64")
endif()

file(GLOB GENICAM_SO_FILES "${GENICAM_LIB_DIR}/*.so*")

# Link dependencies
find_package(spdlog CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        pthread
        spdlog::spdlog_header_only
        ${GENICAM_SO_FILES}
)

# Set rpath for GenICam libraries
set_target_properties(${PROJECT_NAME} PROPERTIES
    BUILD_RPATH "${GENICAM_LIB_DIR}"
    INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib/genicam"
)

# Export all symbols for C API
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE
        -fvisibility=default
        -Wall
        -Wextra
        -Wpedantic
    )
endif()

# Install the producer library to standard GenTL location
install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION lib/genicam/gentl
    RUNTIME DESTINATION lib/genicam/gentl
)

# Install headers for development (optional)
install(FILES ${GENTL_HEADERS}
    DESTINATION include/genicam/gentl
)

# Create GenTL Producer info file
set(GENTL_INFO_FILE "${CMAKE_CURRENT_BINARY_DIR}/FpgaCXP.cti.info")
file(WRITE ${GENTL_INFO_FILE}
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<GenTLProducerInfo>
    <Producer>
        <ID>FpgaCXP</ID>
        <Vendor>CameraService</Vendor>
        <Model>FPGA CoaXPress Transport Layer</Model>
        <Version>1.0.0</Version>
        <TLType>CXP</TLType>
        <Name>FpgaCXPProducer</Name>
        <PathName>/usr/lib/genicam/gentl/FpgaCXP.cti</PathName>
        <DisplayName>FPGA CoaXPress Producer</DisplayName>
        <GenTLVersionMajor>1</GenTLVersionMajor>
        <GenTLVersionMinor>6</GenTLVersionMinor>
    </Producer>
</GenTLProducerInfo>
")

install(FILES ${GENTL_INFO_FILE}
    DESTINATION lib/genicam/gentl
)

message(STATUS "GenTL Producer library configured: ${PROJECT_NAME}")
