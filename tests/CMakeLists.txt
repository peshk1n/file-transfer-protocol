if (NOT ANDROID)
    add_executable(test_transfer
        basic_transfer.cpp
    )
    target_link_libraries(test_transfer PRIVATE
        file-transfer-protocol
        transfer_simulation
    )
    target_compile_definitions(test_transfer PRIVATE
        TEST_DATA_DIR="${PROJECT_SOURCE_DIR}/tests/data"
    )
endif()