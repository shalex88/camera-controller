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
    OUTPUT_NAME "libFpgaCXP"
    SUFFIX ".cti"
    PREFIX ""
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

message(STATUS "GenTL Producer library configured: ${PROJECT_NAME}")
