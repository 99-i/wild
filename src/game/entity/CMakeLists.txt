add_library(entity 
    entity_metadata.cpp
    entity.cpp
	player.cpp
)

target_link_libraries(entity ZLIB::ZLIB)

