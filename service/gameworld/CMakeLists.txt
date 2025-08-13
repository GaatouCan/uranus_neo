file(GLOB_RECURSE GAME_WORLD_SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

if (MSVC)
    foreach(file ${GAME_WORLD_SRC})
        get_filename_component(PARENT_DIR "${file}" DIRECTORY)
        string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "" GROUP "${PARENT_DIR}")
        string(REPLACE "/" "\\" GROUP "${GROUP}")
        set(GROUP "${GROUP}")
        source_group("${GROUP}" FILES "${file}")
    endforeach()
endif ()

add_library(gameworld SHARED ${GAME_WORLD_SRC})
target_compile_definitions(gameworld PRIVATE URANUS_SERVICE)

target_link_libraries(gameworld PUBLIC core)
target_link_libraries(gameworld PUBLIC proto_static)
target_link_libraries(gameworld PUBLIC EnTT::EnTT)

target_include_directories(gameworld PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../protobuf/gen)
target_include_directories(gameworld PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../generated)