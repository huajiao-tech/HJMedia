# the minimum version of CMake.
cmake_minimum_required(VERSION 3.15.0)
project(XComponent)

set(NATIVERENDER_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})
add_definitions(-DOHOS_PLATFORM)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}/manager
    ${CMAKE_CURRENT_SOURCE_DIR}/render
)

# HJ core export sits at demo/faceu/harmony/Export/core relative to this file.
# Normalize to absolute paths so find_library resolves correctly from any build dir.
get_filename_component(HJ_CORE_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../core ABSOLUTE)
set(HJ_CORE_INCLUDE ${HJ_CORE_ROOT}/include)
set(HJ_CORE_LIB_DIR ${HJ_CORE_ROOT}/lib)

message(status "HJ_CORE_LIB_DIR=${HJ_CORE_LIB_DIR}")
set(HJ_RENDER_SO_PATH ${HJ_CORE_LIB_DIR}/libHJRender.so)


file(GLOB UI_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/common/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/manager/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/render/*.cpp"
)

file(GLOB UI_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/common/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/manager/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/render/*.h"
)
add_library(nativerender SHARED ${UI_SOURCES} ${UI_HEADERS})

find_library(
    # Sets the name of the path variable.
    EGL-lib
    # Specifies the name of the NDK library that
    # you want CMake to locate.
    EGL
)

find_library(
    # Sets the name of the path variable.
    GLES-lib
    # Specifies the name of the NDK library that
    # you want CMake to locate.
    GLESv3
)

find_library(
    # Sets the name of the path variable.
    hilog-lib
    # Specifies the name of the NDK library that
    # you want CMake to locate.
    hilog_ndk.z
)

find_library(
    # Sets the name of the path variable.
    libace-lib
    # Specifies the name of the NDK library that
    # you want CMake to locate.
    ace_ndk.z
)

find_library(
    # Sets the name of the path variable.
    libnapi-lib
    # Specifies the name of the NDK library that
    # you want CMake to locate.
    ace_napi.z
)

find_library(
    # Sets the name of the path variable.
    libuv-lib
    # Specifies the name of the NDK library that
    # you want CMake to locate.
    uv
)
unset(HJRender_LIB CACHE)

#use this or NO_CMAKE_FIND_ROOT_PATH
#    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
#    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
#    set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)

# if (EXISTS ${HJ_RENDER_SO_PATH})
#     set(HJRender_LIB ${HJ_RENDER_SO_PATH})
# else()
    find_library(
        HJRender_LIB
        NAMES HJRender
        PATHS ${HJ_CORE_LIB_DIR}
        NO_DEFAULT_PATH
        NO_CMAKE_FIND_ROOT_PATH
    )
    if (NOT HJRender_LIB)
        message(FATAL_ERROR "HJRender library not found under ${HJ_CORE_LIB_DIR}")
    endif()
# endif()
#
target_include_directories(nativerender PRIVATE ${HJ_CORE_INCLUDE})

target_link_libraries(nativerender PUBLIC
    ${EGL-lib} ${GLES-lib} ${hilog-lib} ${libace-lib} ${libnapi-lib} ${libuv-lib} ${HJRender_LIB})
