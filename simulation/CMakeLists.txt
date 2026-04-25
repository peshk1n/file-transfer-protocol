add_library(transfer_simulation STATIC
    simulation.cpp
    fake_channel.cpp
)

target_include_directories(transfer_simulation PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(transfer_simulation PUBLIC file-transfer-protocol)