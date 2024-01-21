include(CTest)

include_directories(${PROJECT_SOURCE_DIR}/include )
include_directories(${PROJECT_SOURCE_DIR}/lib/asio-1.28.0/include)
link_libraries(net game Ws2_32 wsock32)

add_subdirectory(server)

add_custom_target(test COMMAND ${CMAKE_CTEST_COMMAND} DEPENDS
    server
)