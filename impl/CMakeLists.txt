list(APPEND CMAKE_PREFIX_PATH ${THIRD_LIBRARY_DIR}/mongo-cxx-driver)

find_package(mongocxx CONFIG REQUIRED)
find_package(bsoncxx CONFIG REQUIRED)

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
target_link_libraries(impl PUBLIC mongo::mongocxx_shared mongo::bsoncxx_shared)
