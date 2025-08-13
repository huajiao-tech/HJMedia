# find_package(PkgConfig QUIET)
# if(PKG_CONFIG_FOUND)
#   pkg_check_modules(FFMPEG QUIET ffmpeg)
# endif()

set(MOD_NAME FFMPEG)
set(FFMPEG_MODULE_NAMES 
    avcodec
    avdevice
    avfilter
    avformat
    avutil
    swresample
    swscale
)
if(WINDOWS)
    set(FFMPEG_MODULE_D_NAMES 
        avcodecd
        avdeviced
        avfilterd
        avformatd
        avutild
        swresampled
        swscaled
    )
endif()

find_path(
    FFMPEG_INCLUDE_DIR
    NAMES config.h
    HINTS ENV FFMPEG_PATH ${FFMPEG_PATH} ${CMAKE_SOURCE_DIR}/${FFMPEG_PATH} ${FFMPEG_INCLUDE_DIRS}
    PATHS ${HJ_DEPS_DIR}/ffmpeg
    PATH_SUFFIXES include)
get_filename_component(FFMPEG_DIR ${FFMPEG_INCLUDE_DIR} DIRECTORY)
# message(STATUS "FFMPEG_INCLUDE_DIR=${FFMPEG_INCLUDE_DIR}, FFMPEG_DIR:${FFMPEG_DIR}")

function(HJ_FIND_FFMPEG_LIBS MOD_LIBS)
    set(${MOD_LIBS})
    foreach(MOD ${ARGN})
        # message(STATUS "MOD:${MOD}")
        unset(MOD_LIB CACHE)
        find_library(
            MOD_LIB
            NAMES ${MOD} lib${MOD}
            HINTS ${HJ_DEPS_DIR}/ffmpeg ${HJ_DEPS_DIR}/ffmpeg/lib
            PATH_SUFFIXES ${ARCHS_NAME})
        # message(STATUS "MOD_LIB=${MOD_LIB}")
        list(APPEND ${MOD_LIBS} ${MOD_LIB})
    endforeach()
    unset(MOD_LIB CACHE)
    set(${MOD_LIBS} ${${MOD_LIBS}} PARENT_SCOPE)
endfunction()
#
HJ_FIND_FFMPEG_LIBS(FFMPEG_LIBS ${FFMPEG_MODULE_NAMES})
if(WINDOWS)
    HJ_FIND_FFMPEG_LIBS(FFMPEG_D_LIBS ${FFMPEG_MODULE_D_NAMES})
    list(APPEND FFMPEG_LIBS ${FFMPEG_D_LIBS})
endif()
# message(STATUS "FFMPEG_LIBS:${FFMPEG_LIBS}")
#
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LIBFFMPEG DEFAULT_MSG FFMPEG_INCLUDE_DIR FFMPEG_LIBS)
mark_as_advanced(FFMPEG_INCLUDE_DIR FFMPEG_LIBS)
#
if(LIBFFMPEG_FOUND)
    set(LIBFFMPEG_INCLUDE_DIRS ${FFMPEG_INCLUDE_DIR})
    set(LIBFFMPEG_LIBRARIES ${FFMPEG_LIBS})
    # message(STATUS "LIBFFMPEG_INCLUDE_DIRS=${LIBFFMPEG_INCLUDE_DIRS}, LIBFFMPEG_LIBRARIES:${LIBFFMPEG_LIBRARIES}")

    if(NOT TARGET LIBFFMPEG)
        add_library(LIBFFMPEG INTERFACE)
        #
        foreach(var ${FFMPEG_MODULE_NAMES})
            # message(STATUS "var:${var}")
            HJ_MATCH_LIB(outvar ${var} ${LIBFFMPEG_LIBRARIES})
            HJ_MATCH_LIB(outvar_d ${var}d ${LIBFFMPEG_LIBRARIES})
            # message(STATUS "outvar:${outvar}, outvar_d:${outvar_d}")
            if(NOT outvar_d) 
                target_link_libraries(LIBFFMPEG INTERFACE ${outvar})
                # message(STATUS "release:${outvar}")
            else()
                target_link_libraries(LIBFFMPEG INTERFACE debug ${outvar_d} optimized ${outvar})
                # message(STATUS "debug:${outvar_d}, release:${outvar}")
            endif()
        endforeach()
    endif()
    # set_target_properties(LIBFFMPEG PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${LIBFFMPEG_INCLUDE_DIRS})
    target_include_directories(LIBFFMPEG INTERFACE ${LIBFFMPEG_INCLUDE_DIRS})
endif()