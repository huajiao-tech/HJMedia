set(PROJ_NAME HJOpencv)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/utils)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/VisualData)

if(WINDOWS)
    list(APPEND ${PROJ_NAME}_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/utils ${CMAKE_CURRENT_SOURCE_DIR}/VisualData)
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

add_library(${PROJ_NAME} STATIC ${SOURCE_FILES} ${HEADER_FILES})

target_link_libraries(${PROJ_NAME} PRIVATE HJComp HJConvert HJUtils LIBSPDLOG LIBYUV LIBSTB LIBREFLCPP yyjson LIBOPENCV)

target_include_directories(${PROJ_NAME} INTERFACE ${${PROJ_NAME}_INCLUDE_DIRS})


HJ_CONFIG_FRAMEWORK(${PROJ_NAME})

set_target_properties(${PROJ_NAME} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${ARCHIVE_output}
    LIBRARY_OUTPUT_DIRECTORY ${library_output}
    RUNTIME_OUTPUT_DIRECTORY ${runtime_output})
	
ProjFolder(${PROJ_NAME} "MainUI")

if(WINDOWS)
    set(OPENCV_FFMPEG_DLL ${HJ_DEPS_DIR}/opencv/lib/${ARCHS_NAME}/opencv_videoio_ffmpeg4130_64.dll)

    set(OPENCV_RELEASE_DLLS)
    foreach(opencv_lib IN LISTS LIBOPENCV_LIBRARIES)
        get_filename_component(_lib_dir "${opencv_lib}" DIRECTORY)
        get_filename_component(_lib_name "${opencv_lib}" NAME_WE)
        list(APPEND OPENCV_RELEASE_DLLS "${_lib_dir}/${_lib_name}.dll")
    endforeach()

    set(OPENCV_DEBUG_DLLS)
    foreach(opencv_lib IN LISTS LIBOPENCVd_LIBRARIES)
        get_filename_component(_lib_dir "${opencv_lib}" DIRECTORY)
        get_filename_component(_lib_name "${opencv_lib}" NAME_WE)
        list(APPEND OPENCV_DEBUG_DLLS "${_lib_dir}/${_lib_name}.dll")
    endforeach()

    if(EXISTS ${OPENCV_FFMPEG_DLL})
        list(APPEND OPENCV_RELEASE_DLLS ${OPENCV_FFMPEG_DLL})
        list(APPEND OPENCV_DEBUG_DLLS ${OPENCV_FFMPEG_DLL})
    endif()
    message (STATUS "runtime_output:${runtime_output}")
    copy_files_to_dir_post_build(
        ${PROJ_NAME}
        "${OPENCV_DEBUG_DLLS}"
        "${OPENCV_RELEASE_DLLS}"
        "${runtime_output}/$<CONFIG>"
    )
endif()

# Copy resource directory to output directory
#copy_directory_post_build(${PROJ_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/../../resource resource)
#copy_directory_post_build(${PROJ_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/data data)
message (STATUS "PROJ_NAME: ${PROJ_NAME}")
