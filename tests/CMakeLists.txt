add_executable(test_transfer
    basic_transfer.cpp
 "fake_channel.cpp")

target_link_libraries(test_transfer
    file-transfer-protocol
)

target_include_directories(test_transfer PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)