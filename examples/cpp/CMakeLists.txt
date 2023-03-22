cmake_minimum_required(VERSION 3.16)

project(TestGRPC)

# find protobuf
find_package(Protobuf CONFIG REQUIRED)
message(STATUS "Using protobuf ${Protobuf_VERSION}")

# Find gRPC installation
# Looks for gRPCConfig.cmake file installed by gRPC's cmake installation.
find_package(gRPC CONFIG REQUIRED)
message(STATUS "Using gRPC ${gRPC_VERSION}")

find_package(PprzgcsGRCP CONFIG REQUIRED)

add_executable(greeter_client "greeter_client.cpp")

target_link_libraries(greeter_client
    PprzgcsGRCP
    gRPC::grpc++_reflection
    gRPC::grpc++
    protobuf::libprotobuf
)

