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

#***only headers*******************************************************************************#
find_package(LIBSPDLOG REQUIRED)
find_package(LIBMINICORO REQUIRED)
find_package(LIBMINIAUDIO REQUIRED)
find_package(LIBSTB REQUIRED)
find_package(LIBCPPHTTPLIB REQUIRED)

#****binaries**********************************************************************************#
find_package(LIBYUV REQUIRED)
find_package(LIBOPENSSL REQUIRED)
find_package(LIBFFMPEG REQUIRED)
find_package(LIBMBEDTLS REQUIRED)
# find_package(LIBSQLITE REQUIRED)
find_package(LIBSQLITEORM REQUIRED)

if(WINDOWS OR MACOSX)
    find_package(LIBGLFW REQUIRED)
    find_package(LIBGTEST REQUIRED)
endif()

if(ANDROID OR HarmonyOS)
    find_package(LibX264 REQUIRED)
    # find_package(LIBZLIB REQUIRED)
endif()
