find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

set(PROTO_SRC_DIR "${CMAKE_SOURCE_DIR}/proto")
set(PROTO_OUT_DIR "${CMAKE_BINARY_DIR}/api/proto")
file(GLOB_RECURSE PROTO_FILES CONFIGURE_DEPENDS "${PROTO_SRC_DIR}/*.proto")

add_library(proto ${PROTO_FILES})

target_link_libraries(proto
        PUBLIC
        protobuf::protobuf
        gRPC::grpc++
)

target_include_directories(proto
        PUBLIC
        ${CMAKE_BINARY_DIR}
        ${Protobuf_INCLUDE_DIRS}
        ${gRPC_INCLUDE_DIRS}
)

get_target_property(grpc_cpp_plugin_location gRPC::grpc_cpp_plugin LOCATION)
protobuf_generate(TARGET proto
        LANGUAGE cpp
        PROTOC_OUT_DIR ${PROTO_OUT_DIR}
)
protobuf_generate(TARGET proto
        LANGUAGE grpc
        GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
        PLUGIN "protoc-gen-grpc=${grpc_cpp_plugin_location}"
        PROTOC_OUT_DIR ${PROTO_OUT_DIR}
)