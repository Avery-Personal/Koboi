cmake_minimum_required(VERSION 3.80) # Someone said this is the best standard; never used CMake before so I'll trust them blindly.

project(KoboiC)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/compiler
    ${CMAKE_SOURCE_DIR}/drivers
)

file(GLOB_RECURSE COMPILER_SOURCES CONFIGURE_DEPENDS
    compiler/*.c
)

file(GLOB_RECURSE DRIVER_SOURCES CONFIGURE_DEPENDS
    drivers/*.c
)

add_library(KDrivers STATIC
    ${DRIVER_SOURCES}
)

add_library(KoboiC STATIC
    ${COMPILER_SOURCES}
)

add_executable(Koboi
    compiler/CLI/Koboi.c
)

target_link_libraries(Koboi
    KoboiC
    KDrivers
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(Koboi PRIVATE DEBUG)
endif()

set_target_properties(Koboi PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build
)
