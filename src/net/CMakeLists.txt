include(CMakePrintHelpers)

add_subdirectory(packet)
add_subdirectory(crypto)

add_library(net 
    client.cpp
    server.cpp
)

target_link_libraries(net PRIVATE packet crypto gmp)
