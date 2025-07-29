cmake_minimum_required(VERSION 3.31)
project(uranus VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
if (MSVC)
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebugDLL")  # /MDd
    else()
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")       # /MD
    endif()
endif ()

if (WIN32)
    add_compile_definitions(_WIN32_WINNT=0x0A00)
    option(USE_SOLUTION_FOLDERS ON)
endif ()

add_compile_definitions(ASIO_STANDALONE)
add_compile_definitions(ASIO_HAS_CO_AWAIT)

# Import Third Library
set(THIRD_LIBRARY_DIR D:/library/install)
set(CMAKE_INSTALL_PREFIX D:/library/install/uranus)

list(APPEND CMAKE_PREFIX_PATH ${THIRD_LIBRARY_DIR}/spdlog)
list(APPEND CMAKE_PREFIX_PATH ${THIRD_LIBRARY_DIR}/asio)
list(APPEND CMAKE_PREFIX_PATH ${THIRD_LIBRARY_DIR}/nlohmann_json)
list(APPEND CMAKE_PREFIX_PATH ${THIRD_LIBRARY_DIR}/zlib)
list(APPEND CMAKE_PREFIX_PATH ${THIRD_LIBRARY_DIR}/YAML_CPP)
list(APPEND CMAKE_PREFIX_PATH ${THIRD_LIBRARY_DIR}/openssl)
list(APPEND CMAKE_PREFIX_PATH ${THIRD_LIBRARY_DIR}/protobuf)

if (UNIX)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif ()

find_package(spdlog CONFIG REQUIRED)
find_package(asio CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(OpenSSL COMPONENTS REQUIRED)
find_package(Protobuf CONFIG REQUIRED)

# The Core Library
file(GLOB_RECURSE URANUS_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)
file(GLOB_RECURSE URANUS_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
file(GLOB_RECURSE URANUS_FILES  ${URANUS_HEADER} ${URANUS_SOURCE})

if (MSVC)
    foreach(file ${URANUS_FILES})
        get_filename_component(PARENT_DIR "${file}" DIRECTORY)
        string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "" GROUP "${PARENT_DIR}")
        string(REPLACE "/" "\\" GROUP "${GROUP}")
        set(GROUP "${GROUP}")
        source_group("${GROUP}" FILES "${file}")
    endforeach()
endif ()

add_library(core SHARED ${URANUS_FILES})
target_compile_definitions(core PRIVATE URANUS_EXPORT)
target_include_directories(core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(core PUBLIC spdlog::spdlog)
target_link_libraries(core PUBLIC asio::asio)
target_link_libraries(core PUBLIC nlohmann_json::nlohmann_json)
target_link_libraries(core PUBLIC yaml-cpp::yaml-cpp)
target_link_libraries(core PUBLIC OpenSSL::Crypto)
target_link_libraries(core PUBLIC OpenSSL::SSL)

# Add Protobuf Static
add_subdirectory(protobuf)

# Add Default Implement
add_subdirectory(impl)

add_executable(uranus Uranus.cpp)
target_link_libraries(uranus PUBLIC core)
target_link_libraries(uranus PUBLIC impl)

target_include_directories(uranus PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include>
)
target_include_directories(uranus PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/impl)
