if(ANDROID OR HarmonyOS)
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
    set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
endif()

find_program(PROTOBUF_PROTOC_EXECUTABLE 
                NAMES protoc 
                PATHS ${HJ_DEPS_DIR}/tools)

find_program(GRPC_CPP_PLUGIN_EXECUTABLE 
                NAMES grpc_cpp_plugin 
                PATHS ${HJ_DEPS_DIR}/tools)
# message(STATUS "PROTOBUF_PROTOC_EXECUTABLE:${PROTOBUF_PROTOC_EXECUTABLE}, GRPC_CPP_PLUGIN_EXECUTABLE:${GRPC_CPP_PLUGIN_EXECUTABLE}")


if("${PLATFORM_DIR}" STREQUAL "mac" AND "${ARCHS_NAME}" STREQUAL "arm64-v8a")
    find_package(LIBSPDLOG REQUIRED)
    find_package(LIBSTB REQUIRED)
    find_package(LIBYUV REQUIRED)
    find_package(LIBMINICORO REQUIRED)
    find_package(LIBREFLCPP REQUIRED)
else()
    #***only headers*******************************************************************************#
    find_package(LIBSPDLOG REQUIRED)
    find_package(LIBMINICORO REQUIRED)
    find_package(LIBMINIAUDIO REQUIRED)
    find_package(LIBSTB REQUIRED)
    find_package(LIBCPPHTTPLIB REQUIRED)
    find_package(LIBREFLCPP REQUIRED)
    #****binaries**********************************************************************************#
    find_package(LIBYUV REQUIRED)
    find_package(LIBOPENSSL REQUIRED)

    if(CMAKE_GENERATOR_PLATFORM STREQUAL "Win32")
        find_package(LIBFFMPEG QUIET)
        find_package(LIBMBEDTLS QUIET)
    else()
        find_package(LIBFFMPEG REQUIRED)
        find_package(LIBMBEDTLS REQUIRED) 
    endif()
    
    # find_package(LIBSQLITE REQUIRED)
    find_package(LIBSQLITEORM REQUIRED)

    if (WINDOWS)
        if(CMAKE_GENERATOR_PLATFORM STREQUAL "Win32")
            find_package(LIBTNN QUIET)
            find_package(LIBNCNN QUIET)
            find_package(LIBOPENCV QUIET)
        else()
            if (HJ_ENABLE_TNN)
                find_package(LIBTNN REQUIRED)
            endif()
            find_package(LIBOPENCV REQUIRED)
        endif()
    endif()

    if(WINDOWS OR MACOSX)
        # find_package(LIBGLFW REQUIRED)
        if(CMAKE_GENERATOR_PLATFORM STREQUAL "Win32")
            find_package(LIBGTEST QUIET)
        else()
            find_package(LIBGTEST REQUIRED)
        endif()
        find_package(LIBCURL REQUIRED)
    endif()

    if(WINDOWS OR ANDROID OR IOS OR HarmonyOS)
        find_package(LIBNCNN REQUIRED)
    endif()


    # if(IOS OR HarmonyOS)
    #     find_package(LIBX264 REQUIRED)
    #     # find_package(LIBZLIB REQUIRED)
    # endif()

    if(HarmonyOS)
        find_package(LIBMINDSPORE REQUIRED)
        find_package(LIBX264 REQUIRED)
        # find_package(LIBZLIB REQUIRED)
    endif()


    if(MACOSX)
        find_package(LIBX264 REQUIRED)
        find_package(LIBX265 REQUIRED)
        find_package(LIBSRT REQUIRED)
    endif()
endif()

