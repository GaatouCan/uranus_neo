file(GLOB_RECURSE IMPL_SOURCE_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

if (MSVC)
    foreach(file ${IMPL_SOURCE_FILES})
        get_filename_component(PARENT_DIR "${file}" DIRECTORY)
        string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "" GROUP "${PARENT_DIR}")
        string(REPLACE "/" "\\" GROUP "${GROUP}")
        set(GROUP "${GROUP}")
        source_group("${GROUP}" FILES "${file}")
    endforeach()
endif ()

add_library(impl SHARED ${IMPL_SOURCE_FILES})
target_compile_definitions(impl PRIVATE BUILD_IMPL_LIBRARY)
target_link_libraries(impl PUBLIC core)
target_link_libraries(impl PUBLIC proto_static)

target_include_directories(impl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../src)