if (CMAKE_CROSSCOMPILING)
    find_program(PROTOBUF_PROTOC protoc)
else ()
    set(PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)
endif ()

file(GLOB_RECURSE PROTOBUF_LIST ${CMAKE_CURRENT_SOURCE_DIR}/def/*.proto)

foreach (proto ${PROTOBUF_LIST})
    get_filename_component(proto_name ${proto} NAME_WE)
    set(proto_src ${CMAKE_CURRENT_SOURCE_DIR}/gen/${proto_name}.pb.cc)
    set(proto_hdr ${CMAKE_CURRENT_SOURCE_DIR}/gen/${proto_name}.pb.h)
    add_custom_command(
            OUTPUT ${proto_src} ${proto_hdr}
            COMMAND ${PROTOBUF_PROTOC}
            ARGS --cpp_out ${CMAKE_CURRENT_SOURCE_DIR}/gen -I ${CMAKE_CURRENT_SOURCE_DIR}/def ${proto}
            DEPENDS ${proto})

    list(APPEND PROTOBUF_FILES ${proto_src} ${proto_hdr})
endforeach ()

add_library(proto_static STATIC ${PROTOBUF_FILES})
target_link_libraries(proto_static PUBLIC protobuf::libprotobuf-lite)
target_include_directories(proto_static PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/gen>
        $<INSTALL_INTERFACE:include>
)

set_target_properties(proto_static PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set_target_properties(proto_static PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set_target_properties(proto_static PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)
