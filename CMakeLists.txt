cmake_minimum_required(VERSION 3.16)
project(file-transfer-protocol)

set(CMAKE_CXX_STANDARD 17)

add_library(file-transfer-protocol STATIC
    src/session.cpp
 "include/transfer/packets.h" "src/transfer_agent.h" "src/sender.h" "src/receiver.h")

target_include_directories(file-transfer-protocol PUBLIC include)