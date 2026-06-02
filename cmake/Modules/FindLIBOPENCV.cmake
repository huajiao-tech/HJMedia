set(PROJ_NAME OPENCV)
set(LIBPROJ_NAME LIB${PROJ_NAME})
set(PROJ_LIB_NAME opencv)
set(PROJ_DIR_NAME opencv)
#set(PROJ_HEADER blob_converter.h)

# find_package(PkgConfig QUIET)
# if(PKG_CONFIG_FOUND)
#   pkg_check_modules(${PROJ_NAME} QUIET ${PROJ_LIB_NAME})
# endif()

# find_path(
#   ${PROJ_NAME}_INCLUDE_DIR
#   NAMES ${PROJ_HEADER}
#   HINTS ${HJ_DEPS_DIR}/${PROJ_DIR_NAME}
#   PATH_SUFFIXES include/ncnn)
# get_filename_component(${PROJ_NAME}_DIR ${${PROJ_NAME}_INCLUDE_DIR} DIRECTORY)

if (WINDOWS)
      get_filename_component(OPENCV_ROOT "${HJ_DEPS_DIR}/${PROJ_DIR_NAME}" ABSOLUTE)
      set(${PROJ_NAME}_INCLUDE_DIR ${OPENCV_ROOT}/include)
      set(OPENCV_LIB_DIR ${OPENCV_ROOT}/lib/${ARCHS_NAME})

      file(GLOB OPENCV_RELEASE_LIBS "${OPENCV_LIB_DIR}/opencv_*.lib")
      list(FILTER OPENCV_RELEASE_LIBS EXCLUDE REGEX ".*d\\.lib$")
      list(SORT OPENCV_RELEASE_LIBS)

      file(GLOB OPENCV_DEBUG_LIBS "${OPENCV_LIB_DIR}/opencv_*d.lib")
      list(SORT OPENCV_DEBUG_LIBS)

      set(${PROJ_NAME}_LIB "")
      set(${PROJ_NAME}d_LIB "")
      if (OPENCV_RELEASE_LIBS)
        list(GET OPENCV_RELEASE_LIBS 0 ${PROJ_NAME}_LIB)
      endif()
      if (OPENCV_DEBUG_LIBS)
        list(GET OPENCV_DEBUG_LIBS 0 ${PROJ_NAME}d_LIB)
      endif()

      if (${PROJ_NAME}_LIB)
        get_filename_component(${PROJ_NAME}_LIB_DIR ${${PROJ_NAME}_LIB} DIRECTORY)
      endif()
      if (${PROJ_NAME}d_LIB)
        get_filename_component(${PROJ_NAME}d_LIB_DIR ${${PROJ_NAME}d_LIB} DIRECTORY)
      endif()
      message(STATUS "OPENCV_ROOT=${OPENCV_ROOT}, ${PROJ_NAME}_INCLUDE_DIR=${${PROJ_NAME}_INCLUDE_DIR}")
      message(STATUS "${PROJ_NAME}_LIB=${${PROJ_NAME}_LIB} ${PROJ_NAME}_LIB_DIR=${${PROJ_NAME}_LIB_DIR}")
      message(STATUS "${PROJ_NAME}d_LIB=${${PROJ_NAME}d_LIB} ${PROJ_NAME}d_LIB_DIR=${${PROJ_NAME}d_LIB_DIR}")
else()
      set(${PROJ_NAME}_INCLUDE_DIR ${HJ_DEPS_DIR}/${PROJ_DIR_NAME}/include)
      message(STATUS "HJ_DEPS_DIR=${HJ_DEPS_DIR}, ${PROJ_NAME}_INCLUDE_DIR=${${PROJ_NAME}_INCLUDE_DIR}, ${PROJ_NAME}_DIR:${${PROJ_NAME}_DIR}")

      find_library(
        ${PROJ_NAME}_LIB
        NAMES ${${PROJ_NAME}_LIBRARIES} ${PROJ_LIB_NAME} lib${PROJ_LIB_NAME}
        HINTS ${HJ_DEPS_DIR}/${PROJ_DIR_NAME} ${HJ_DEPS_DIR}/${PROJ_DIR_NAME}/lib
        PATH_SUFFIXES ${ARCHS_NAME})
      get_filename_component(${PROJ_NAME}_LIB_DIR ${${PROJ_NAME}_LIB} DIRECTORY)
      message(STATUS "${PROJ_NAME}_LIB=${${PROJ_NAME}_LIB} ${PROJ_NAME}_LIB_DIR=${${PROJ_NAME}_LIB_DIR}")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(${LIBPROJ_NAME} DEFAULT_MSG ${PROJ_NAME}_INCLUDE_DIR ${PROJ_NAME}_LIB)
mark_as_advanced(${PROJ_NAME}_INCLUDE_DIR ${PROJ_NAME}_LIB)

if(${LIBPROJ_NAME}_FOUND)
  set(${LIBPROJ_NAME}_INCLUDE_DIRS ${${PROJ_NAME}_INCLUDE_DIR})
  if (WINDOWS)
    set(${LIBPROJ_NAME}_LIBRARIES ${OPENCV_RELEASE_LIBS})
    set(${LIBPROJ_NAME}d_LIBRARIES ${OPENCV_DEBUG_LIBS})
  else()
    set(${LIBPROJ_NAME}_LIBRARIES ${${PROJ_NAME}_LIB})
  endif()
  message(STATUS "${LIBPROJ_NAME}_INCLUDE_DIRS=${${LIBPROJ_NAME}_INCLUDE_DIRS}, ${LIBPROJ_NAME}_LIBRARIES:${${LIBPROJ_NAME}_LIBRARIES}")

  add_library(${LIBPROJ_NAME} INTERFACE)

  if (WINDOWS)
    target_link_libraries(${LIBPROJ_NAME} INTERFACE
      $<$<CONFIG:Debug>:${${LIBPROJ_NAME}d_LIBRARIES}>
      $<$<NOT:$<CONFIG:Debug>>:${${LIBPROJ_NAME}_LIBRARIES}>
    )
    message(STATUS "interface debug ${LIBPROJ_NAME}d_LIBRARIES:${${LIBPROJ_NAME}d_LIBRARIES}")
    message(STATUS "interface release ${LIBPROJ_NAME}_LIBRARIES:${${LIBPROJ_NAME}_LIBRARIES}")
  elseif (MSVC)
    target_link_libraries(${LIBPROJ_NAME} INTERFACE
      $<$<CONFIG:Debug>:${${LIBPROJ_NAME}d_LIBRARIES}>
      $<$<NOT:$<CONFIG:Debug>>:${${LIBPROJ_NAME}_LIBRARIES}>
    )
  else()
    target_link_libraries(${LIBPROJ_NAME} INTERFACE ${${LIBPROJ_NAME}_LIBRARIES})
  endif()

  target_include_directories(${LIBPROJ_NAME} INTERFACE ${${LIBPROJ_NAME}_INCLUDE_DIRS})
  target_compile_definitions(${LIBPROJ_NAME} INTERFACE BUILDING_DLL)
endif()
