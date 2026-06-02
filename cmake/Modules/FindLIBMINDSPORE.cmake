set(PROJ_NAME MINDSPORE)
set(LIBPROJ_NAME LIB${PROJ_NAME})
set(PROJ_DIR_NAME mindspore)
set(PROJ_LIB_NAME mindspore-lite)

set(_MINDSPORE_ROOT_HINTS
    ${HJ_DEPS_DIR}/${PROJ_DIR_NAME}
    ${HJ_DEPS_DIR}/${PROJ_DIR_NAME}/runtime)

find_path(
  ${PROJ_NAME}_INCLUDE_DIR
  NAMES include/api/model.h
  HINTS ${_MINDSPORE_ROOT_HINTS})

set(${PROJ_NAME}_LIB "")

if(HarmonyOS)
  set(_MINDSPORE_STATIC_CANDIDATES
    "${HJ_DEPS_DIR}/${PROJ_DIR_NAME}/runtime/lib/lib${PROJ_LIB_NAME}.a"
    "${HJ_DEPS_DIR}/${PROJ_DIR_NAME}/lib/${ARCHS_NAME}/lib${PROJ_LIB_NAME}.a"
    "${HJ_DEPS_DIR}/${PROJ_DIR_NAME}/lib/lib${PROJ_LIB_NAME}.a")
  foreach(_mindspore_static_lib IN LISTS _MINDSPORE_STATIC_CANDIDATES)
    if(EXISTS "${_mindspore_static_lib}")
      set(${PROJ_NAME}_LIB "${_mindspore_static_lib}")
      break()
    endif()
  endforeach()
endif()

if(NOT ${PROJ_NAME}_LIB)
  find_library(
    ${PROJ_NAME}_LIB
    NAMES ${PROJ_LIB_NAME} lib${PROJ_LIB_NAME}
    HINTS
      ${HJ_DEPS_DIR}/${PROJ_DIR_NAME}
      ${HJ_DEPS_DIR}/${PROJ_DIR_NAME}/lib
      ${HJ_DEPS_DIR}/${PROJ_DIR_NAME}/runtime
      ${HJ_DEPS_DIR}/${PROJ_DIR_NAME}/runtime/lib
    PATH_SUFFIXES ${ARCHS_NAME})
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(${LIBPROJ_NAME} DEFAULT_MSG ${PROJ_NAME}_INCLUDE_DIR ${PROJ_NAME}_LIB)
mark_as_advanced(${PROJ_NAME}_INCLUDE_DIR ${PROJ_NAME}_LIB)

if(${LIBPROJ_NAME}_FOUND)
  set(${LIBPROJ_NAME}_INCLUDE_DIRS ${${PROJ_NAME}_INCLUDE_DIR})
  set(${LIBPROJ_NAME}_LIBRARIES ${${PROJ_NAME}_LIB})
  get_filename_component(_mindspore_lib_ext "${${LIBPROJ_NAME}_LIBRARIES}" EXT)

  add_library(${LIBPROJ_NAME} INTERFACE)
  if(HarmonyOS AND _mindspore_lib_ext STREQUAL ".a")
    # MindSpore Lite registers kernels and populate creators through static initializers.
    # When linked as a normal static library, Harmony's linker may discard those
    # object files, which later surfaces as "Unsupported parameter type in Create".
    target_link_libraries(${LIBPROJ_NAME} INTERFACE
      "-Wl,--whole-archive"
      ${${LIBPROJ_NAME}_LIBRARIES}
      "-Wl,--no-whole-archive")
  else()
    target_link_libraries(${LIBPROJ_NAME} INTERFACE ${${LIBPROJ_NAME}_LIBRARIES})
  endif()
  target_include_directories(${LIBPROJ_NAME} INTERFACE ${${LIBPROJ_NAME}_INCLUDE_DIRS})
endif()
