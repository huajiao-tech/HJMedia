#**********************************************************************************#
#Please use them everywhere
#WINDOWS   	=   Windows Desktop
#ANDROID    =  Android
#IOS    	=  iOS
#MACOSX    	=  MacOS X
#LINUX      =   Linux
#**********************************************************************************#
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    add_definitions(-DWINDOWS)
    set(WINDOWS TRUE)
    set(PLATFORM_FOLDER wsys)
    set(PLATFORM_DIR windows)
	MESSAGE(STATUS "current platform: Windows") 
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Android")
    set(PLATFORM_FOLDER asys)
    set(PLATFORM_DIR android)
	MESSAGE(STATUS "current platform: Android")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    if(ANDROID)
        set(PLATFORM_FOLDER asys)
        set(PLATFORM_DIR android)
		MESSAGE(STATUS "current platform: Android")
    else()
        set(LINUX TRUE)
        set(PLATFORM_FOLDER lsys)
        set(PLATFORM_DIR linux)
		MESSAGE(STATUS "current platform: Linux ") 
    endif()
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(APPLE TRUE)
    set(MACOSX TRUE)
    set(PLATFORM_FOLDER msys)
    set(PLATFORM_DIR mac)
	MESSAGE(STATUS "current platform: Mac OS X") 
elseif(${CMAKE_SYSTEM_NAME} MATCHES "iOS")
    set(APPLE TRUE)
    set(IOS TRUE)
    set(PLATFORM_FOLDER isys)
    set(PLATFORM_DIR ios)
	MESSAGE(STATUS "current platform: IOS") 
elseif(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
    set(FREEBSD TRUE)
	set(PLATFORM_FOLDER fsys)
    set(PLATFORM_DIR FreeBSD)
	MESSAGE(STATUS "current platform: FreeBSD")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "OHOS")
    add_definitions(-DHarmonyOS)
    set(HarmonyOS TRUE)
    set(PLATFORM_DIR harmony)
else()
    message(FATAL_ERROR "Unsupported platform, CMake will exit")
    return()
endif()

# generators that are capable of organizing into a hierarchy of folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
# simplify generator condition, please use them everywhere
if(CMAKE_GENERATOR STREQUAL Xcode)
    set(XCODE TRUE)
elseif(CMAKE_GENERATOR MATCHES Visual)
    set(VS TRUE)
endif()
message(STATUS "CMAKE_GENERATOR: ${CMAKE_GENERATOR}")

# if(MSVC)
# 	add_definitions( /wd4251 /wd4275 /wd4819 /wd4099 /wd4244 /wd4996 /wd4101)
# endif()

#**********************************************************************************#
# retrieve files, dir and sub dirs
#**********************************************************************************#
function(HJ_CHECK_SUBSTR result str)
    foreach(substr ${ARGN})
        # message (STATUS "str:${str} substr:${substr}")
        set(valid)
        string(FIND ${str} ${substr} valid REVERSE)
        if(${valid} GREATER_EQUAL 0)
            # message (STATUS "valid result:yes")
            set(${result} TRUE PARENT_SCOPE)
            return()
        endif()
    endforeach()
    set(${result} FALSE PARENT_SCOPE)
    # return("NO")
endfunction()

#**********************************************************************************#
# c, cpp, cc, m, mm....
#**********************************************************************************#
function(HJ_RETRIEVE_XFILES out_files)
    set(source_list)
    foreach(dirname ${ARGN})
        # message (STATUS "dirname:${dirname}")
        file(GLOB_RECURSE files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
			"${dirname}/*.cmake"
            "${dirname}/*.hpp"
            "${dirname}/*.c"
            "${dirname}/*.cpp"
            "${dirname}/*.cc"
            "${dirname}/*.m"
            "${dirname}/*.mm"
        )
        foreach(filename ${files})
            get_filename_component(source_path "${filename}" PATH)
            string(REPLACE "/" "\\" filter_dir "${source_path}")
            # message(STATUS "source path:${source_path}")
            set(invalid_dir FALSE)
            if(WIN32)
                HJ_CHECK_SUBSTR(invalid_dir ${filter_dir} "asys" "isys" "msys" "osys" "lsys" "hsys" "test")
            elseif(IOS)
                HJ_CHECK_SUBSTR(invalid_dir ${filter_dir} "asys" "wsys" "msys" "lsys" "hsys" "test")
            elseif(MACOSX)
                HJ_CHECK_SUBSTR(invalid_dir ${filter_dir} "asys" "isys" "wsys" "lsys" "hsys" "test")
            elseif(ANDROID)
                HJ_CHECK_SUBSTR(invalid_dir ${filter_dir} "wsys" "isys" "msys" "osys" "lsys" "hsys" "test")
            elseif(HarmonyOS)
                HJ_CHECK_SUBSTR(invalid_dir ${filter_dir} "asys" "wsys" "isys" "msys" "osys" "lsys" "test")
            endif()
            if(NOT invalid_dir)
                set(file_abs_name "${CMAKE_CURRENT_SOURCE_DIR}/${filename}")
                # message(STATUS "valid dir: ${file_abs_name}")
                list(APPEND source_list "${file_abs_name}")			
                source_group("${filter_dir}" FILES "${filename}")
            endif()
        endforeach()
    endforeach()
    set(${out_files} ${source_list} PARENT_SCOPE)
endfunction()

#**********************************************************************************#
# h
#**********************************************************************************#
function(HJ_RETRIEVE_HFILES out_files)
    set(source_list)
    foreach(dirname ${ARGN})
        # message (STATUS "dirname:${dirname}")
        file(GLOB_RECURSE files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
            "${dirname}/*.h"
        )
        foreach(filename ${files})
            get_filename_component(source_path "${filename}" PATH)
            string(REPLACE "/" "\\" filter_dir "${source_path}")
            # message(STATUS "source path:${source_path}")
            set(invalid_dir FALSE)
            if(WIN32)
                HJ_CHECK_SUBSTR(invalid_dir ${filter_dir} "asys" "isys" "msys" "osys" "lsys" "hsys" "test")
            elseif(IOS)
                HJ_CHECK_SUBSTR(invalid_dir ${filter_dir} "asys" "wsys" "msys" "lsys" "hsys" "test")
            elseif(MACOSX)
                HJ_CHECK_SUBSTR(invalid_dir ${filter_dir} "asys" "isys" "wsys" "lsys" "hsys" "test")
            elseif(ANDROID)
                HJ_CHECK_SUBSTR(invalid_dir ${filter_dir} "wsys" "isys" "msys" "osys" "lsys" "hsys" "test")
            elseif(HarmonyOS)
                HJ_CHECK_SUBSTR(invalid_dir ${filter_dir} "asys" "wsys" "isys" "msys" "osys" "lsys" "test")
            endif()
            if(NOT invalid_dir)
                set(file_abs_name "${CMAKE_CURRENT_SOURCE_DIR}/${filename}")
                # message(STATUS "valid dir: ${file_abs_name}")
                list(APPEND source_list "${file_abs_name}")			
                source_group("${filter_dir}" FILES "${filename}")
            endif()
        endforeach()
    endforeach()
    set(${out_files} ${source_list} PARENT_SCOPE)
endfunction()

MACRO(HJ_SUBDIR_LIST result curdir)
  FILE(GLOB children RELATIVE ${curdir} ${curdir}/*)
  SET(dirlist "")
  FOREACH(child ${children})
    IF(IS_DIRECTORY ${curdir}/${child})
      LIST(APPEND dirlist ${child})
    ENDIF()
  ENDFOREACH()
  SET(${result} ${dirlist})
ENDMACRO() 

#**********************************************************************************#
# Debug & Release
#**********************************************************************************#
set(BUILD_DEBUG FALSE)
if(CMAKE_BUILD_TYPE AND (CMAKE_BUILD_TYPE STREQUAL "Debug"))
    set(BUILD_DEBUG TRUE)
    message(STATUS "build mode:${CMAKE_C_FLAGS_DEBUG}")
elseif(CMAKE_BUILD_TYPE AND (CMAKE_BUILD_TYPE STREQUAL "Release"))
    message(STATUS "build mode:${CMAKE_C_FLAGS_RELEASE}")
else()
    message(STATUS "build:${CMAKE_BUILD_TYPE} cflags:${CMAKE_C_FLAGS_RELEASE}")
endif()

#**********************************************************************************#
# x64 x86 arm64 armv7
#**********************************************************************************#
# message(STATUS "detected processor: ${CMAKE_SYSTEM_PROCESSOR}")
if(CMAKE_SYSTEM_PROCESSOR MATCHES "amd64.*|x86_64.*|AMD64.*")
    set(ARCHS_NAME "x64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "i686.*|i386.*|x86.*")
    set(ARCHS_NAME "x86")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64.*|AARCH64.*|arm64.*|ARM64.*)")
    # set(ARCHS_NAME "arm64")
    set(ARCHS_NAME "arm64-v8a")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm.*|ARM.*)")
    # set(ARCHS_NAME "armv7")
    set(ARCHS_NAME "armeabi-v7a")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(powerpc|ppc)64le")
    set(ARCHS_NAME "p64le")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(powerpc|ppc)64")
    set(ARCHS_NAME "p64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(mips.*|MIPS.*)")
    set(ARCHS_NAME "mips")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(riscv.*|RISCV.*)")
    set(ARCHS_NAME "riscv")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(loongarch64.*|LOONGARCH64.*)")
    set(ARCHS_NAME "lg64")
else()
    message(WARNING "unrecognized target processor configuration")
endif()
message(STATUS "detected ARCHS_NAME:${ARCHS_NAME}")

# if(WINDOWS)
#     if(CMAKE_CL_64)
#         set(ARCHS_NAME "x64")
#     else()
#         set(ARCHS_NAME "x86")
#     endif()
# elseif(ANDROID)
#     if (${ANDROID_ABI} MATCHES "armeabi-v7a")
#         set(ARCHS_NAME "armv7")
#     elseif(${ANDROID_ABI} MATCHES "arm64-v8a")
#         set(ARCHS_NAME "arm64")
#     elseif(${ANDROID_ABI} MATCHES "x86")
#         set(ARCHS_NAME "x86")
#     elseif(${ANDROID_ABI} MATCHES "x86_64")
#         set(ARCHS_NAME "x64")
#     endif()
# elseif(LINUX)
# elseif(MACOSX)
# elseif(IOS)
#     if (${PLATFORM} MATCHES "OS")
#         set(ARCHS_NAME "armv7")
#     elseif(${PLATFORM} MATCHES "OS64")
#         set(ARCHS_NAME "arm64")
#     elseif(${PLATFORM} MATCHES "SIMULATOR")
#         set(ARCHS_NAME "x86")
#     elseif(${PLATFORM} MATCHES "SIMULATOR64")
#         set(ARCHS_NAME "x64")
#     endif()
# endif()

#**********************************************************************************#
# config framework
#**********************************************************************************#
function(HJ_CONFIG_FRAMEWORK LIBNAME)
    set(PROP_PLIST_FILE "${CMAKE_SOURCE_DIR}/cmake/isys/Info.plist")

    if (APPLE) #HJ_BUILD_LIBS_AS_FRAMEWORKS
        set_target_properties(${LIBNAME} PROPERTIES 
            FRAMEWORK TRUE
            XCODE_ATTRIBUTE_INFO_PLIST "${PROP_PLIST_FILE}"
            XCODE_RESOURCE "${RESOURCE_FILES}"
            # RESOURCE "${RESOURCE_FILES}"
            # MACOSX_BUNDLE_RESOURCES "${RESOURCE_FILES}"
            # FRAMEWORK_VERSION "1.9.12"
            # MACOSX_FRAMEWORK_BUNDLE_NAME ${LIBNAME}
        )
        MESSAGE( STATUS "PROP_PLIST_FILE:${PROP_PLIST_FILE} RESOURCE_FILES:${RESOURCE_FILES}")

        # Set the INSTALL_PATH so that frameworks can be installed in the application package
        set_target_properties(${LIBNAME}
            PROPERTIES BUILD_WITH_INSTALL_RPATH 1
            INSTALL_NAME_DIR "@executable_path/../Frameworks"
        )
        #   set_target_properties(${LIBNAME} PROPERTIES PUBLIC_HEADER "${HEADER_FILES};" )
        set_target_properties(${LIBNAME} PROPERTIES PUBLIC_HEADER "${PUBLIC_HEADERS}" )
        set_target_properties(${LIBNAME} PROPERTIES RESOURCE "${RESOURCE_FILES}")
        set_source_files_properties("${RESOURCE_FILES}" PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

        set_target_properties(${LIBNAME} PROPERTIES OUTPUT_NAME ${LIBNAME})
    endif()
endfunction(HJ_CONFIG_FRAMEWORK)

#**********************************************************************************#
# build proto, grpc. not work ????
#**********************************************************************************#
function(HJ_PROTOC_COMMAND SRC_FILES HDR_FILES)
    if(NOT ARGN)
        message(SEND_ERROR "Error: HJ_PROTOC_COMMAND() called without any proto files")
        return()
    endif()
    set(${SRC_FILES})
    set(${HDR_FILES})

    foreach(FIL ${ARGN})
        get_filename_component(ABS_FIL ${FIL} ABSOLUTE)
        get_filename_component(FIL_WE ${FIL} NAME_WE)
        get_filename_component(REL_DIR ${ABS_FIL} DIRECTORY)
        
        message(STATUS "ABS_FIL:${ABS_FIL}, REL_DIR:${REL_DIR}, FIL_WE:${FIL_WE}")

        set(GRPC_PROTO_GENS_DIR ${REL_DIR}/gens)
        file(MAKE_DIRECTORY ${GRPC_PROTO_GENS_DIR})

        set(PB_SRCS ${GRPC_PROTO_GENS_DIR}/${FIL_WE}.pb.cc)
        set(PB_HDRS ${GRPC_PROTO_GENS_DIR}/${FIL_WE}.pb.h)
        set(GRPC_SRCS ${GRPC_PROTO_GENS_DIR}/${FIL_WE}.grpc.pb.cc)
        set(GRPC_HDRS ${GRPC_PROTO_GENS_DIR}/${FIL_WE}.grpc.pb.h)
        message(STATUS "PB_SRCS:${PB_SRCS}, PB_HDRS:${PB_HDRS}, GRPC_SRCS:${GRPC_SRCS}, GRPC_HDRS:${GRPC_HDRS}")
    
        list(APPEND ${SRC_FILES} ${PB_SRCS})
        list(APPEND ${HDR_FILES} ${PB_HDRS})
        list(APPEND ${SRC_FILES} ${GRPC_SRCS})
        list(APPEND ${HDR_FILES} ${GRPC_HDRS})

        add_custom_command(
            OUTPUT ${PB_SRCS} ${PB_HDRS} ${GRPC_SRCS} ${GRPC_HDRS}
            COMMAND ${PROTOBUF_PROTOC_EXECUTABLE}
            ARGS --grpc_out=${GRPC_PROTO_GENS_DIR}
                --cpp_out=${GRPC_PROTO_GENS_DIR}
                --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN_EXECUTABLE}
                -I ${REL_DIR}
                ${ABS_FIL}
            WORKING_DIRECTORY ${GRPC_PROTO_GENS_DIR}
            DEPENDS ${PROTOBUF_PROTOC_EXECUTABLE} ${GRPC_CPP_PLUGIN_EXECUTABLE} ${ABS_FIL} 
            COMMAND "Running C++ protocol buffer compiler on ${FIL}")
    endforeach()

    set_source_files_properties(${${SRC_FILES}} ${${HDR_FILES}} PROPERTIES GENERATED TRUE)
    set(${SRC_FILES} ${${SRC_FILES}} PARENT_SCOPE)
    set(${HDR_FILES} ${${HDR_FILES}} PARENT_SCOPE)
endfunction(HJ_PROTOC_COMMAND)

# file(GLOB PROTO_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.proto")
# HJ_PROTOC_COMMAND(PB_SRC_FILES PB_HDR_FILES ${PROTO_FILES})

#**********************************************************************************#
# common build config
#**********************************************************************************#
function(MAKE_COMMON_CONFIG)
    if(MSVC)
        # message(STATUS "make config set definition")
        add_definitions( /wd4251 /wd4275 /wd4819 /wd4099 /wd4244 /wd4996 /wd4101 /wd5033)
    endif()
    if (APPLE)
        cmake_policy(SET CMP0068 NEW)
        if(ARGN)
            message(STATUS "make common config set properties -- objective-c -- ARGN:${ARGN}")
            set_source_files_properties(${ARGN} PROPERTIES COMPILE_FLAGS "-x objective-c++ -fobjc-arc")
        endif()
    endif()
endfunction(MAKE_COMMON_CONFIG)

#**********************************************************************************#
# match lib
#**********************************************************************************#
function(HJ_MATCH_LIB out in)
    set(${out})
    foreach(var ${ARGN})
        get_filename_component(varname ${var} NAME_WE)
        # message(STATUS "varname:${varname}, in:${in}")
        if((${varname} STREQUAL ${in}) OR (${varname} STREQUAL lib${in}))
            # message(STATUS "find varname:${varname}, in:${in}")
            set(${out} ${var})
            break()
        endif()
    endforeach()
    set(${out} ${${out}} PARENT_SCOPE)
endfunction()