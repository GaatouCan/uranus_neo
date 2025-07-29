file(GLOB_RECURSE PLAYER_AGENT_SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

if (MSVC)
    foreach(file ${PLAYER_AGENT_SRC})
        get_filename_component(PARENT_DIR "${file}" DIRECTORY)
        string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "" GROUP "${PARENT_DIR}")
        string(REPLACE "/" "\\" GROUP "${GROUP}")
        set(GROUP "${GROUP}")
        source_group("${GROUP}" FILES "${file}")
    endforeach()
endif ()

add_library(agent SHARED ${PLAYER_AGENT_SRC} ${ORM_SOURCE_FILES})
target_compile_definitions(agent PRIVATE URANUS_SERVICE)

target_link_libraries(agent PUBLIC core)
target_link_libraries(agent PUBLIC impl)
target_link_libraries(agent PUBLIC proto_static)

target_include_directories(agent PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../impl)
target_include_directories(agent PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../protobuf/gen)
target_include_directories(agent PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../generated)
