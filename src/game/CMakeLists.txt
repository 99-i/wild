add_subdirectory(world)
add_subdirectory(entity)

add_library(game 
    game.cpp
)

target_link_libraries(game entity world)

