set(PROJ_NAME HJPlayer)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${HJ_EXTERNALS_PATH}/generic/ffmpeg)

if (HarmonyOS)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/hsys/bridge)
endif()

set(${PROJ_NAME}_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR})

# message (STATUS "retrieve_files entry")
HJ_SUBDIR_LIST(subdir_names ${CMAKE_CURRENT_SOURCE_DIR})
# message (STATUS "core sub dir names:${subdir_names}")

HJ_RETRIEVE_XFILES(SUBPROJ_SOURCE_FILES ${subdir_names})
HJ_RETRIEVE_HFILES(SUBPROJ_HEADER_FILES ${subdir_names})

file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cc" "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/../*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/../*.cc")
list(APPEND HEADER_FILES ${SUBPROJ_HEADER_FILES})
list(APPEND SOURCE_FILES ${SUBPROJ_SOURCE_FILES})

# list(APPEND SOURCE_FILES ${CMAKE_SOURCE_DIR}/Dependencies/generic/spdlog/details/registry-inl.cc)

MAKE_COMMON_CONFIG()

add_library(${PROJ_NAME} SHARED ${SOURCE_FILES} ${HEADER_FILES})

if (HarmonyOS)
    set(Harmony_Native_LIBRARY
    libace_napi.z.so libEGL.so libGLESv3.so libace_ndk.z.so libuv.so libhilog_ndk.z.so
    libnative_media_codecbase.so libnative_media_core.so libnative_media_vdec.so libnative_window.so
    libnative_media_venc.so libnative_media_acodec.so libnative_media_avdemuxer.so libnative_media_avsource.so libnative_media_avmuxer.so
    libohaudio.so libnative_image.so libnative_buffer.so)
    target_link_libraries(${PROJ_NAME} PRIVATE ${Harmony_Native_LIBRARY})

    target_link_libraries(${PROJ_NAME} PRIVATE LIBX264)
    target_link_libraries(${PROJ_NAME} PRIVATE LIBZLIB)   # ffmpeg png encoder need zlib
    target_link_options(${PROJ_NAME} PRIVATE "-Wl,-Bsymbolic")  #ld.lld: error: relocation R_AARCH64_ADR_PREL_PG_HI21 cannot be used against symbol 'ff_tx_tab_2048_float'; recompile with -fPIC
endif()

target_link_libraries(${PROJ_NAME} PRIVATE HJLinks HJGraphs HJPlugins HJComp HJMedia yyjson libfdkaac HJStat LIBFFMPEG LIBSTB LIBSPDLOG LIBMINICORO HJUtils)
target_include_directories(${PROJ_NAME} INTERFACE ${${PROJ_NAME}_INCLUDE_DIRS})

HJ_CONFIG_FRAMEWORK(${PROJ_NAME})

set_target_properties(${PROJ_NAME} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${ARCHIVE_output}
    LIBRARY_OUTPUT_DIRECTORY ${library_output}
    RUNTIME_OUTPUT_DIRECTORY ${runtime_output})
	
ProjFolder(${PROJ_NAME} "src")