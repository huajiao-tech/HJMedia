set(PROJ_NAME NCNN)
set(LIBPROJ_NAME LIB${PROJ_NAME})
set(PROJ_LIB_NAME ncnn)
set(PROJ_GLSLANG_LIB_NAME glslang)
set(PROJ_SPIRV_LIB_NAME SPIRV)
set(PROJ_OPENMP_LIB_NAME openmp)
set(PROJ_MOLTENVK_LIB_NAME MoltenVK)
set(PROJ_DIR_NAME ncnn)
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

set(${PROJ_NAME}_INCLUDE_DIR ${HJ_DEPS_DIR}/${PROJ_DIR_NAME}/include)
message(STATUS "HJ_DEPS_DIR=${HJ_DEPS_DIR}, ${PROJ_NAME}_INCLUDE_DIR=${${PROJ_NAME}_INCLUDE_DIR}, ${PROJ_NAME}_DIR:${${PROJ_NAME}_DIR}")

if(IOS)
  set(${PROJ_NAME}_MOLTENVK_LIB "" CACHE FILEPATH "Optional absolute path to MoltenVK binary for iOS NCNN Vulkan backend")
  # iOS package layouts supported:
  # 1) framework bundle: ${HJ_DEPS_DIR}/ncnn/ncnn.framework
  # 2) static libs + headers: ${HJ_DEPS_DIR}/ncnn/lib + ${HJ_DEPS_DIR}/ncnn/include
  set(_NCNN_IOS_ROOT ${HJ_DEPS_DIR}/${PROJ_DIR_NAME})

  function(_hj_pick_ios_framework_binary out_var fw_root fw_name)
    set(_framework_root "${fw_root}/${fw_name}.framework")
    set(_cands
        "${_framework_root}/Versions/Current/${fw_name}"
        "${_framework_root}/Versions/A/${fw_name}"
        "${_framework_root}/Verision/Current/${fw_name}"
        "${_framework_root}/Verision/A/${fw_name}"
        "${_framework_root}/Version/Current/${fw_name}"
        "${_framework_root}/Version/A/${fw_name}"
        "${_framework_root}/${fw_name}"
        "${_framework_root}/Versions/Current/lib${fw_name}.a"
        "${_framework_root}/Versions/A/lib${fw_name}.a"
        "${_framework_root}/Verision/Current/lib${fw_name}.a"
        "${_framework_root}/Verision/A/lib${fw_name}.a"
        "${_framework_root}/Version/Current/lib${fw_name}.a"
        "${_framework_root}/Version/A/lib${fw_name}.a"
        "${_framework_root}/lib${fw_name}.a"
        "${_framework_root}/Versions/Current/${fw_name}.tbd"
        "${_framework_root}/Versions/A/${fw_name}.tbd"
        "${_framework_root}/Verision/Current/${fw_name}.tbd"
        "${_framework_root}/Verision/A/${fw_name}.tbd"
        "${_framework_root}/${fw_name}.tbd")
    set(_picked "")
    foreach(_cand IN LISTS _cands)
      if(EXISTS "${_cand}")
        set(_picked "${_cand}")
        break()
      endif()
    endforeach()
    if(NOT _picked AND IS_DIRECTORY "${_framework_root}")
      # Some third-party packages keep the binary in Versions/A but with a
      # non-standard filename. Pick the first linkable artifact as fallback.
      file(GLOB _bins
          "${_framework_root}/Versions/A/*"
          "${_framework_root}/Verision/A/*"
          "${_framework_root}/Version/A/*"
          "${_framework_root}/Versions/A/*.a"
          "${_framework_root}/Verision/A/*.a"
          "${_framework_root}/Versions/A/*.dylib"
          "${_framework_root}/Verision/A/*.dylib"
          "${_framework_root}/Versions/A/*.tbd"
          "${_framework_root}/Verision/A/*.tbd"
          "${_framework_root}/Version/A/*.a"
          "${_framework_root}/Version/A/*.dylib"
          "${_framework_root}/Version/A/*.tbd")
      foreach(_bin IN LISTS _bins)
        if(IS_DIRECTORY "${_bin}")
          continue()
        endif()
        get_filename_component(_bin_name "${_bin}" NAME)
        if(_bin_name STREQUAL "Info.plist")
          continue()
        endif()
        set(_picked "${_bin}")
        break()
      endforeach()
    endif()
    if(NOT _picked)
      # Fallback to static/shared library layout when framework isn't present.
      find_library(_picked
        NAMES ${fw_name} lib${fw_name}
        HINTS "${fw_root}" "${fw_root}/lib"
        PATH_SUFFIXES ${ARCHS_NAME}
        NO_DEFAULT_PATH)
    endif()
    if(NOT _picked)
      message(STATUS "LIBNCNN: failed to locate ${fw_name} binary. Checked framework root ${_framework_root}")
    endif()
    set(${out_var} "${_picked}" PARENT_SCOPE)
  endfunction()

  _hj_pick_ios_framework_binary(${PROJ_NAME}_LIB "${_NCNN_IOS_ROOT}" "${PROJ_LIB_NAME}")
  _hj_pick_ios_framework_binary(${PROJ_NAME}_GLSLANG_LIB "${_NCNN_IOS_ROOT}" "${PROJ_GLSLANG_LIB_NAME}")
  _hj_pick_ios_framework_binary(${PROJ_NAME}_SPIRV_LIB "${_NCNN_IOS_ROOT}" "${PROJ_SPIRV_LIB_NAME}")
  if(NOT ${PROJ_NAME}_SPIRV_LIB)
    _hj_pick_ios_framework_binary(${PROJ_NAME}_SPIRV_LIB "${_NCNN_IOS_ROOT}" "${PROJ_OPENMP_LIB_NAME}")
  endif()
  if(${PROJ_NAME}_MOLTENVK_LIB AND EXISTS "${${PROJ_NAME}_MOLTENVK_LIB}")
    set(${PROJ_NAME}_MOLTENVK_LIB_RESOLVED "${${PROJ_NAME}_MOLTENVK_LIB}")
  else()
    _hj_pick_ios_framework_binary(${PROJ_NAME}_MOLTENVK_LIB_RESOLVED "${_NCNN_IOS_ROOT}" "${PROJ_MOLTENVK_LIB_NAME}")
    if(NOT ${PROJ_NAME}_MOLTENVK_LIB_RESOLVED)
      _hj_pick_ios_framework_binary(${PROJ_NAME}_MOLTENVK_LIB_RESOLVED "${HJ_DEPS_DIR}" "${PROJ_MOLTENVK_LIB_NAME}")
    endif()
    if(NOT ${PROJ_NAME}_MOLTENVK_LIB_RESOLVED)
      _hj_pick_ios_framework_binary(${PROJ_NAME}_MOLTENVK_LIB_RESOLVED "${HJ_DEPS_DIR}/moltenvk" "${PROJ_MOLTENVK_LIB_NAME}")
    endif()
  endif()

  # Some packaged frameworks expose Headers as a symlink/alias that may be unusable
  # on certain filesystems. Probe common real paths in order.
  set(_NCNN_IOS_INCLUDE_CANDIDATES
      "${_NCNN_IOS_ROOT}/ncnn.framework/Headers"
      "${_NCNN_IOS_ROOT}/ncnn.framework/Versions/Current/Headers"
      "${_NCNN_IOS_ROOT}/ncnn.framework/Versions/A/Headers"
      "${_NCNN_IOS_ROOT}/ncnn.framework/Verision/Current/Headers"
      "${_NCNN_IOS_ROOT}/ncnn.framework/Verision/A/Headers"
      "${_NCNN_IOS_ROOT}/include"
      "${_NCNN_IOS_ROOT}/include/ncnn")
  foreach(_cand IN LISTS _NCNN_IOS_INCLUDE_CANDIDATES)
    if(IS_DIRECTORY "${_cand}")
      set(${PROJ_NAME}_INCLUDE_DIR "${_cand}")
      break()
    endif()
  endforeach()
  if(NOT IS_DIRECTORY "${${PROJ_NAME}_INCLUDE_DIR}")
    message(WARNING "NCNN iOS headers not found under ${_NCNN_IOS_ROOT}")
  endif()

  # Keep framework search path available for transitive -framework flags.
  set(${PROJ_NAME}_FRAMEWORK_SEARCH_DIR "${_NCNN_IOS_ROOT}")
else()
  find_library(
    ${PROJ_NAME}_LIB
    NAMES ${${PROJ_NAME}_LIBRARIES} ${PROJ_LIB_NAME} lib${PROJ_LIB_NAME}
    HINTS ${HJ_DEPS_DIR}/${PROJ_DIR_NAME} ${HJ_DEPS_DIR}/${PROJ_DIR_NAME}/lib
    PATH_SUFFIXES ${ARCHS_NAME})

  find_library(
    ${PROJ_NAME}_GLSLANG_LIB
    NAMES ${PROJ_GLSLANG_LIB_NAME} lib${PROJ_GLSLANG_LIB_NAME}
    HINTS ${HJ_DEPS_DIR}/${PROJ_DIR_NAME} ${HJ_DEPS_DIR}/${PROJ_DIR_NAME}/lib
    PATH_SUFFIXES ${ARCHS_NAME})

  find_library(
    ${PROJ_NAME}_SPIRV_LIB
    NAMES ${PROJ_SPIRV_LIB_NAME} lib${PROJ_SPIRV_LIB_NAME}
    HINTS ${HJ_DEPS_DIR}/${PROJ_DIR_NAME} ${HJ_DEPS_DIR}/${PROJ_DIR_NAME}/lib
    PATH_SUFFIXES ${ARCHS_NAME})
endif()

if(${PROJ_NAME}_LIB)
  get_filename_component(${PROJ_NAME}_LIB_DIR "${${PROJ_NAME}_LIB}" DIRECTORY)
else()
  set(${PROJ_NAME}_LIB_DIR "")
  message(WARNING "${PROJ_NAME}_LIB not found under ${HJ_DEPS_DIR}/${PROJ_DIR_NAME}")
endif()
message(STATUS "${PROJ_NAME}_LIB=${${PROJ_NAME}_LIB} ${PROJ_NAME}_LIB_DIR=${${PROJ_NAME}_LIB_DIR}")
message(STATUS "${PROJ_NAME}_GLSLANG_LIB=${${PROJ_NAME}_GLSLANG_LIB}")
message(STATUS "${PROJ_NAME}_SPIRV_LIB=${${PROJ_NAME}_SPIRV_LIB}")
if(IOS)
  if(${PROJ_NAME}_MOLTENVK_LIB_RESOLVED)
    message(STATUS "${PROJ_NAME}_MOLTENVK_LIB=${${PROJ_NAME}_MOLTENVK_LIB_RESOLVED}")
  else()
    message(WARNING "MoltenVK not found. If linking fails with _vkGetInstanceProcAddr, set -D${PROJ_NAME}_MOLTENVK_LIB=/path/to/MoltenVK")
  endif()
endif()

if (WINDOWS)
  find_library(
    ${PROJ_NAME}d_LIB
    NAMES ${${PROJ_NAME}_LIBRARIES} ${PROJ_LIB_NAME}d lib${PROJ_LIB_NAME}d
    HINTS ${HJ_DEPS_DIR}/${PROJ_DIR_NAME} ${HJ_DEPS_DIR}/${PROJ_DIR_NAME}/lib
    PATH_SUFFIXES ${ARCHS_NAME})

  find_library(
    ${PROJ_NAME}d_GLSLANG_LIB
    NAMES ${PROJ_GLSLANG_LIB_NAME}d lib${PROJ_GLSLANG_LIB_NAME}d
    HINTS ${HJ_DEPS_DIR}/${PROJ_DIR_NAME} ${HJ_DEPS_DIR}/${PROJ_DIR_NAME}/lib
    PATH_SUFFIXES ${ARCHS_NAME})

  find_library(
    ${PROJ_NAME}d_SPIRV_LIB
    NAMES ${PROJ_SPIRV_LIB_NAME}d lib${PROJ_SPIRV_LIB_NAME}d
    HINTS ${HJ_DEPS_DIR}/${PROJ_DIR_NAME} ${HJ_DEPS_DIR}/${PROJ_DIR_NAME}/lib
    PATH_SUFFIXES ${ARCHS_NAME})

  get_filename_component(${PROJ_NAME}d_LIB_DIR ${${PROJ_NAME}d_LIB} DIRECTORY)
  message(STATUS "${PROJ_NAME}d_LIB=${${PROJ_NAME}d_LIB} ${PROJ_NAME}d_LIB_DIR=${${PROJ_NAME}d_LIB_DIR}")
  message(STATUS "${PROJ_NAME}d_GLSLANG_LIB=${${PROJ_NAME}d_GLSLANG_LIB}")
  message(STATUS "${PROJ_NAME}d_SPIRV_LIB=${${PROJ_NAME}d_SPIRV_LIB}")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(${LIBPROJ_NAME} DEFAULT_MSG ${PROJ_NAME}_INCLUDE_DIR ${PROJ_NAME}_LIB)
mark_as_advanced(${PROJ_NAME}_INCLUDE_DIR ${PROJ_NAME}_LIB)

if(${LIBPROJ_NAME}_FOUND)
  set(${LIBPROJ_NAME}_INCLUDE_DIRS ${${PROJ_NAME}_INCLUDE_DIR})
  set(_${LIBPROJ_NAME}_LIBS)
  if(${PROJ_NAME}_LIB)
    list(APPEND _${LIBPROJ_NAME}_LIBS ${${PROJ_NAME}_LIB})
  endif()
  if(${PROJ_NAME}_GLSLANG_LIB)
    list(APPEND _${LIBPROJ_NAME}_LIBS ${${PROJ_NAME}_GLSLANG_LIB})
  endif()
  if(${PROJ_NAME}_SPIRV_LIB)
    list(APPEND _${LIBPROJ_NAME}_LIBS ${${PROJ_NAME}_SPIRV_LIB})
  endif()
  if(IOS AND ${PROJ_NAME}_MOLTENVK_LIB_RESOLVED)
    list(APPEND _${LIBPROJ_NAME}_LIBS ${${PROJ_NAME}_MOLTENVK_LIB_RESOLVED})
  endif()
  set(${LIBPROJ_NAME}_LIBRARIES ${_${LIBPROJ_NAME}_LIBS})
  message(STATUS "${LIBPROJ_NAME}_INCLUDE_DIRS=${${LIBPROJ_NAME}_INCLUDE_DIRS}, ${LIBPROJ_NAME}_LIBRARIES:${${LIBPROJ_NAME}_LIBRARIES}")

  add_library(${LIBPROJ_NAME} INTERFACE)

  if (WINDOWS)
    set(${LIBPROJ_NAME}d_LIBRARIES
        ${${PROJ_NAME}d_LIB}
        ${${PROJ_NAME}d_GLSLANG_LIB}
        ${${PROJ_NAME}d_SPIRV_LIB})
  endif()

  if (MSVC)
    target_link_libraries(${LIBPROJ_NAME} INTERFACE debug ${${LIBPROJ_NAME}d_LIBRARIES} optimized ${${LIBPROJ_NAME}_LIBRARIES})
  else()
    target_link_libraries(${LIBPROJ_NAME} INTERFACE ${${LIBPROJ_NAME}_LIBRARIES})
  endif()

  target_include_directories(${LIBPROJ_NAME} INTERFACE ${${LIBPROJ_NAME}_INCLUDE_DIRS})
  if(IOS AND NOT "${${PROJ_NAME}_FRAMEWORK_SEARCH_DIR}" STREQUAL "")
    target_link_options(${LIBPROJ_NAME} INTERFACE "-F${${PROJ_NAME}_FRAMEWORK_SEARCH_DIR}")
  endif()
  target_compile_definitions(${LIBPROJ_NAME} INTERFACE BUILDING_DLL)
endif()
