set(PROJ_NAME TNN)
set(LIBPROJ_NAME LIB${PROJ_NAME})
set(PROJ_LIB_NAME TNN)
set(PROJ_DIR_NAME tnn)
#set(PROJ_HEADER blob_converter.h)

# find_package(PkgConfig QUIET)
# if(PKG_CONFIG_FOUND)
#   pkg_check_modules(${PROJ_NAME} QUIET ${PROJ_LIB_NAME})
# endif()

# find_path(
#   ${PROJ_NAME}_INCLUDE_DIR
#   NAMES ${PROJ_HEADER}
#   HINTS ${HJ_DEPS_DIR}/${PROJ_DIR_NAME}
#   PATH_SUFFIXES include/tnn/utils)
# get_filename_component(${PROJ_NAME}_DIR ${${PROJ_NAME}_INCLUDE_DIR} DIRECTORY)

set(${PROJ_NAME}_INCLUDE_DIR ${HJ_DEPS_DIR}/${PROJ_DIR_NAME}/include)
message(STATUS "HJ_DEPS_DIR=${HJ_DEPS_DIR}, ${PROJ_NAME}_INCLUDE_DIR=${${PROJ_NAME}_INCLUDE_DIR}, ${PROJ_NAME}_DIR:${${PROJ_NAME}_DIR}")

find_library(
  ${PROJ_NAME}_LIB
  NAMES ${${PROJ_NAME}_LIBRARIES} ${PROJ_LIB_NAME} lib${PROJ_LIB_NAME}
  HINTS ${HJ_DEPS_DIR}/${PROJ_DIR_NAME} ${HJ_DEPS_DIR}/${PROJ_DIR_NAME}/lib
  PATH_SUFFIXES ${ARCHS_NAME})
get_filename_component(${PROJ_NAME}_LIB_DIR ${${PROJ_NAME}_LIB} DIRECTORY)
message(STATUS "${PROJ_NAME}_LIB=${${PROJ_NAME}_LIB} ${PROJ_NAME}_LIB_DIR=${${PROJ_NAME}_LIB_DIR}")

if (WINDOWS)
  find_library(
    ${PROJ_NAME}d_LIB
    NAMES ${${PROJ_NAME}_LIBRARIES} ${PROJ_LIB_NAME}d lib${PROJ_LIB_NAME}d
    HINTS ${HJ_DEPS_DIR}/${PROJ_DIR_NAME} ${HJ_DEPS_DIR}/${PROJ_DIR_NAME}/lib
    PATH_SUFFIXES ${ARCHS_NAME})
  get_filename_component(${PROJ_NAME}d_LIB_DIR ${${PROJ_NAME}d_LIB} DIRECTORY)
  message(STATUS "${PROJ_NAME}d_LIB=${${PROJ_NAME}d_LIB} ${PROJ_NAME}d_LIB_DIR=${${PROJ_NAME}d_LIB_DIR}")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(${LIBPROJ_NAME} DEFAULT_MSG ${PROJ_NAME}_INCLUDE_DIR ${PROJ_NAME}_LIB)
mark_as_advanced(${PROJ_NAME}_INCLUDE_DIR ${PROJ_NAME}_LIB)

if(${LIBPROJ_NAME}_FOUND)
  set(${LIBPROJ_NAME}_INCLUDE_DIRS ${${PROJ_NAME}_INCLUDE_DIR})
  set(${LIBPROJ_NAME}_LIBRARIES ${${PROJ_NAME}_LIB})
  message(STATUS "${LIBPROJ_NAME}_INCLUDE_DIRS=${${LIBPROJ_NAME}_INCLUDE_DIRS}, ${LIBPROJ_NAME}_LIBRARIES:${${LIBPROJ_NAME}_LIBRARIES}")

  add_library(${LIBPROJ_NAME} INTERFACE)

  if (WINDOWS)
    set(${LIBPROJ_NAME}d_LIBRARIES ${${PROJ_NAME}d_LIB})
  endif()

  # TNN is a static archive; force WHOLEARCHIVE so the full archive is linked in.
  if (MSVC)
    if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24")
      target_link_libraries(${LIBPROJ_NAME} INTERFACE
        debug $<LINK_LIBRARY:WHOLE_ARCHIVE,${${LIBPROJ_NAME}d_LIBRARIES}>
        optimized $<LINK_LIBRARY:WHOLE_ARCHIVE,${${LIBPROJ_NAME}_LIBRARIES}>)
    else()
      target_link_libraries(${LIBPROJ_NAME} INTERFACE
        debug ${${LIBPROJ_NAME}d_LIBRARIES}
        optimized ${${LIBPROJ_NAME}_LIBRARIES})
      target_link_options(${LIBPROJ_NAME} INTERFACE
        "$<$<CONFIG:Debug>:/WHOLEARCHIVE:${${LIBPROJ_NAME}d_LIBRARIES}>"
        "$<$<NOT:$<CONFIG:Debug>>:/WHOLEARCHIVE:${${LIBPROJ_NAME}_LIBRARIES}>")
    endif()
  else()
    target_link_libraries(${LIBPROJ_NAME} INTERFACE -Wl,--whole-archive ${${LIBPROJ_NAME}_LIBRARIES} -Wl,--no-whole-archive)
  endif()

  target_include_directories(${LIBPROJ_NAME} INTERFACE ${${LIBPROJ_NAME}_INCLUDE_DIRS})
  target_compile_definitions(${LIBPROJ_NAME} INTERFACE BUILDING_DLL)
endif()
