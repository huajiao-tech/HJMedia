set(PROJ_NAME HJMediaPlayer)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../utils)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${HJ_EXTERNALS_PATH}/generic/ffmpeg)
if (WINDOWS OR MACOSX)
    include_directories(${CMAKE_SOURCE_DIR}/third_party/glfw/deps)
endif()

if (HarmonyOS)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/hsys/bridge)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../hsys)
endif()

if (ANDROID)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/asys)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../utils/asys)
endif()

if (IOS)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/isys)
    set(${PROJ_NAME}_INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/isys)
else()
    set(${PROJ_NAME}_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR})
endif()

# message (STATUS "retrieve_files entry")
HJ_SUBDIR_LIST(subdir_names ${CMAKE_CURRENT_SOURCE_DIR})
# message (STATUS "core sub dir names:${subdir_names}")

HJ_RETRIEVE_XFILES(SUBPROJ_SOURCE_FILES ${subdir_names})
HJ_RETRIEVE_HFILES(SUBPROJ_HEADER_FILES ${subdir_names})

file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cc" "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/../*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/../*.cc")
file(GLOB ENTRY_UTILS_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/../utils/*.h")
file(GLOB ENTRY_UTILS_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/../utils/*.cc" "${CMAKE_CURRENT_SOURCE_DIR}/../utils/*.cpp")
list(APPEND HEADER_FILES ${SUBPROJ_HEADER_FILES})
list(APPEND SOURCE_FILES ${SUBPROJ_SOURCE_FILES})
list(APPEND HEADER_FILES ${ENTRY_UTILS_HEADER_FILES})
list(APPEND SOURCE_FILES ${ENTRY_UTILS_SOURCE_FILES})

if (HarmonyOS)
    file(GLOB HEADER_FILES_HSYS "${CMAKE_CURRENT_SOURCE_DIR}/../hsys*.h")
    file(GLOB SOURCE_FILES_HSYS "${CMAKE_CURRENT_SOURCE_DIR}/../hsys/*.cpp")
    list(APPEND HEADER_FILES ${HEADER_FILES_HSYS})
    list(APPEND SOURCE_FILES ${SOURCE_FILES_HSYS})
endif()

if (ANDROID)
    file(GLOB ASYS_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/../utils/asys/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/../utils/asys/*.cc")
    list(APPEND SOURCE_FILES ${ASYS_SOURCE_FILES})
endif()

if (IOS)
    file(GLOB IOS_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/isys/*.mm")
    list(APPEND SOURCE_FILES ${IOS_SOURCE_FILES})

    set(PUBLIC_HEADERS
        "${CMAKE_CURRENT_SOURCE_DIR}/isys/HJMusicPlayer.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/isys/HJPlayerContext.h")
endif()

# list(APPEND SOURCE_FILES ${CMAKE_SOURCE_DIR}/Dependencies/generic/spdlog/details/registry-inl.cc)

MAKE_COMMON_CONFIG()

add_library(${PROJ_NAME} SHARED ${SOURCE_FILES} ${HEADER_FILES})

if (HarmonyOS)
    set(Harmony_Native_LIBRARY
    libace_napi.z.so libdeviceinfo_ndk.z.so libEGL.so libGLESv3.so libace_ndk.z.so libuv.so libhilog_ndk.z.so
    libnative_media_codecbase.so libnative_media_core.so libnative_media_vdec.so libnative_window.so
    libnative_media_venc.so libnative_media_acodec.so libnative_media_avdemuxer.so libnative_media_avsource.so libnative_media_avmuxer.so
    libohaudio.so libnative_image.so libnative_buffer.so libimage_source.so libpixelmap.so libimage_receiver.so libohimage.so)
    target_link_libraries(${PROJ_NAME} PRIVATE ${Harmony_Native_LIBRARY})

    #target_link_libraries(${PROJ_NAME} PRIVATE LIBX264)
    target_link_libraries(${PROJ_NAME} PRIVATE zlib)   # ffmpeg png encoder need zlib
    target_link_options(${PROJ_NAME} PRIVATE "-Wl,-Bsymbolic")  #ld.lld: error: relocation R_AARCH64_ADR_PREL_PG_HI21 cannot be used against symbol 'ff_tx_tab_2048_float'; recompile with -fPIC
endif()

target_link_libraries(${PROJ_NAME} PRIVATE HJDataFuse HJMediaDBKit HJConvert HJLinks HJGraphs HJPlugins HJComp HJMedia yyjson libfdkaac HJStat LIBFFMPEG LIBSTB LIBSPDLOG LIBMINICORO HJUtils LIBREFLCPP)

if (ANDROID)
    target_link_libraries(${PROJ_NAME} PRIVATE PRIVATE mediandk android c m dl z log EGL GLESv3 OpenSLES)
    if (NOT ANDROID_STUDIO_DEBUG)
        set_target_properties(${PROJ_NAME} PROPERTIES LINK_FLAGS -s)
    endif()
elseif (IOS)
    find_library(UIKIT_FRAMEWORK UIKit)
    find_library(FOUNDATION_FRAMEWORK Foundation)
    find_library(COREFOUNDATION_FRAMEWORK CoreFoundation)
    find_library(AVFOUNDATION_FRAMEWORK AVFoundation)
    find_library(AUDIOTOOLBOX_FRAMEWORK AudioToolbox)
    find_library(COREGRAPHICS_FRAMEWORK CoreGraphics)
    find_library(COREIMAGE_FRAMEWORK CoreImage)
    find_library(COREMEDIA_FRAMEWORK CoreMedia)
    find_library(COREVIDEO_FRAMEWORK CoreVideo)
    find_library(VIDEOTOOLBOX_FRAMEWORK VideoToolbox)
    find_library(METAL_FRAMEWORK Metal)
    find_library(OPENGLES_FRAMEWORK OpenGLES)
    find_library(QUARTZCORE_FRAMEWORK QuartzCore)

    target_link_libraries(${PROJ_NAME} PRIVATE
        bz2
        iconv
        ${UIKIT_FRAMEWORK}
        ${FOUNDATION_FRAMEWORK}
        ${COREFOUNDATION_FRAMEWORK}
        ${AVFOUNDATION_FRAMEWORK}
        ${AUDIOTOOLBOX_FRAMEWORK}
        ${COREGRAPHICS_FRAMEWORK}
        ${COREIMAGE_FRAMEWORK}
        ${COREMEDIA_FRAMEWORK}
        ${COREVIDEO_FRAMEWORK}
        ${VIDEOTOOLBOX_FRAMEWORK}
        ${METAL_FRAMEWORK}
        ${OPENGLES_FRAMEWORK}
        ${QUARTZCORE_FRAMEWORK})
endif()

target_include_directories(${PROJ_NAME} INTERFACE ${${PROJ_NAME}_INCLUDE_DIRS})

HJ_CONFIG_FRAMEWORK_EX(${PROJ_NAME})

if (IOS AND CMAKE_GENERATOR STREQUAL "Xcode")
    set_xcode_property(${PROJ_NAME} DEBUG_INFORMATION_FORMAT "dwarf-with-dsym" "Release")
    set_xcode_property(${PROJ_NAME} GCC_GENERATE_DEBUGGING_SYMBOLS "YES" "Release")
endif()

set_target_properties(${PROJ_NAME} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${ARCHIVE_output}
    LIBRARY_OUTPUT_DIRECTORY ${library_output}
    RUNTIME_OUTPUT_DIRECTORY ${runtime_output})
	
ProjFolder(${PROJ_NAME} "src")
