set(PROJ_NAME HJMedia)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/io)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/codec)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/demuxer)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/muxer)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/render)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/capture)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/sei)
include_directories(${HJ_EXTERNALS_PATH}/generic)
include_directories(${HJ_EXTERNALS_PATH}/generic/ffmpeg)

set(${PROJ_NAME}_INCLUDE_DIRS 
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${CMAKE_CURRENT_SOURCE_DIR}/io
    ${CMAKE_CURRENT_SOURCE_DIR}/codec
    ${CMAKE_CURRENT_SOURCE_DIR}/demuxer   
    ${CMAKE_CURRENT_SOURCE_DIR}/muxer
    ${CMAKE_CURRENT_SOURCE_DIR}/render
    ${CMAKE_CURRENT_SOURCE_DIR}/capture
    ${CMAKE_CURRENT_SOURCE_DIR}/sei)
	
if (HarmonyOS)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/capture/hsys)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/render/hsys)
	list(APPEND ${PROJ_NAME}_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/render/hsys)	
    list(APPEND ${PROJ_NAME}_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/codec/hsys)
    list(APPEND ${PROJ_NAME}_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/capture/hsys)
endif()	

# message (STATUS "retrieve_files entry")
HJ_SUBDIR_LIST(subdir_names ${CMAKE_CURRENT_SOURCE_DIR})
# message (STATUS "core sub dir names:${subdir_names}")

HJ_RETRIEVE_XFILES(SUBPROJ_SOURCE_FILES ${subdir_names})
HJ_RETRIEVE_HFILES(SUBPROJ_HEADER_FILES ${subdir_names})

file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cc" "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
list(APPEND HEADER_FILES ${SUBPROJ_HEADER_FILES})
list(APPEND SOURCE_FILES ${SUBPROJ_SOURCE_FILES})

# list(APPEND SOURCE_FILES ${CMAKE_SOURCE_DIR}/Dependencies/generic/spdlog/details/registry-inl.cc)

MAKE_COMMON_CONFIG()

if (APPLE)
  set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/render/HJARenderMini.cc"
    PROPERTIES COMPILE_FLAGS "-x objective-c++ -fobjc-arc")
endif ()

add_library(${PROJ_NAME} STATIC ${SOURCE_FILES} ${HEADER_FILES})

target_link_libraries(${PROJ_NAME} PRIVATE HJDataFuse LIBSQLITEORM sqlite3 HJStat libfdkaac librtmp HJUtils soundtouch sonic lz4 zlib LIBOPENSSL LIBMBEDTLS LIBFFMPEG LIBYUV LIBMINIAUDIO LIBSPDLOG)
target_include_directories(${PROJ_NAME} INTERFACE ${${PROJ_NAME}_INCLUDE_DIRS})

HJ_CONFIG_FRAMEWORK(${PROJ_NAME})

set_target_properties(${PROJ_NAME} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${ARCHIVE_output}
    LIBRARY_OUTPUT_DIRECTORY ${library_output}
    RUNTIME_OUTPUT_DIRECTORY ${runtime_output})
	
ProjFolder(${PROJ_NAME} "src")