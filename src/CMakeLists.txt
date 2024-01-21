add_subdirectory(net)
add_subdirectory(game)

add_executable(wild 
    main.cpp
)

target_link_libraries(wild PRIVATE net game Ws2_32 wsock32)
