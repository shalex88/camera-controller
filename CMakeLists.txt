set(PROJECT_NAME camera-service)
include(cmake/SetVersion.cmake)

if(DEFINED ENV{OECORE_TARGET_SYSROOT} AND DEFINED ENV{CROSS_COMPILE})
    set(CMAKE_CROSSCOMPILING ON)
    message(STATUS "Found Yocto SDK")
endif()

if(CMAKE_CROSSCOMPILING)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
    message(STATUS "Cross compilation")
else()
    message(STATUS "Native compilation")
    include(cmake/EnableVcpkg.cmake)
endif()

cmake_minimum_required(VERSION 3.21)
project(${PROJECT_NAME} LANGUAGES CXX VERSION ${VERSION})

add_compile_definitions(APP_NAME="${PROJECT_NAME}")
add_compile_definitions(GRPC_CALLBACK_API_NONEXPERIMENTAL)

set(CMAKE_CXX_STANDARD 20)

set(SOURCE_DIR "src")
set(INCLUDE_DIR "include")

add_subdirectory(proto)

include_directories(${INCLUDE_DIR} ${SOURCE_DIR} ${proto_INCLUDE_DIRS})
file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS "${SOURCE_DIR}/*.cpp")

find_package(spdlog CONFIG REQUIRED)

find_package(yaml-cpp REQUIRED)
if(TARGET yaml-cpp::yaml-cpp)
    set(YAML_CPP_TARGET yaml-cpp::yaml-cpp)
elseif(TARGET yaml-cpp)
    set(YAML_CPP_TARGET yaml-cpp)
endif()

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

function(link_common_libraries target)
    target_link_libraries(${target}
            PUBLIC
            #ADD your libs here
            proto
            spdlog::spdlog_header_only
            ${YAML_CPP_TARGET}
    )
endfunction(link_common_libraries)

link_common_libraries(${PROJECT_NAME})
add_subdirectory(tests)

include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
include(cmake/CreatePackage.cmake)
