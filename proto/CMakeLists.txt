find_package(Protobuf REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin)
add_compile_definitions(GRPC_CALLBACK_API_NONEXPERIMENTAL)

set(PROTO_SRC_DIR "${CMAKE_SOURCE_DIR}/proto")
set(PROTO_OUT_DIR "${CMAKE_BINARY_DIR}/api/proto")
file(MAKE_DIRECTORY "${PROTO_OUT_DIR}")
file(GLOB_RECURSE PROTO_FILES CONFIGURE_DEPENDS "${PROTO_SRC_DIR}/*.proto")

add_library(proto ${PROTO_FILES})

target_link_libraries(proto
        PUBLIC
        protobuf::libprotobuf
        gRPC::grpc++
        gRPC::grpc++_reflection
)

target_include_directories(proto
        PUBLIC
        ${CMAKE_BINARY_DIR}
        ${Protobuf_INCLUDE_DIRS}
        ${gRPC_INCLUDE_DIRS}
)

protobuf_generate(TARGET proto
        LANGUAGE cpp
        PROTOC_OUT_DIR ${PROTO_OUT_DIR}
)

protobuf_generate(TARGET proto
        LANGUAGE grpc
        GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
        PLUGIN "protoc-gen-grpc=${GRPC_CPP_PLUGIN}"
        PROTOC_OUT_DIR ${PROTO_OUT_DIR}
)