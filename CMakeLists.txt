cmake_minimum_required(VERSION 3.10)
project(device-control)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_executable(device-control
    src/main.cpp
    src/tcp_server.cpp
    src/protocol.cpp
    src/device_manager.cpp
    src/led_device.cpp
    src/key_device.cpp
)

find_package(Threads REQUIRED)

target_link_libraries(device-control 
    PRIVATE Threads::Threads)