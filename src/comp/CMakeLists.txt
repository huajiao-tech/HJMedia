set(PROJ_NAME HJComp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/rte)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/utils)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/graphic)
#include_directories(${HJ_EXTERNALS_PATH}/generic/ffmpeg)

if (HJ_ENABLE_RENDER_PRIO)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/prio)
endif()


list(APPEND ${PROJ_NAME}_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR})
list(APPEND ${PROJ_NAME}_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include)
list(APPEND ${PROJ_NAME}_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/rte)
list(APPEND ${PROJ_NAME}_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/utils)
list(APPEND ${PROJ_NAME}_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/graphic)

if (HJ_ENABLE_RENDER_PRIO)
    list(APPEND ${PROJ_NAME}_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/prio)
endif()

if (HarmonyOS)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/graphic/hsys)
	list(APPEND ${PROJ_NAME}_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/graphic/hsys)
endif()

# message (STATUS "retrieve_files entry")
HJ_SUBDIR_LIST(subdir_names ${CMAKE_CURRENT_SOURCE_DIR})
#essage (STATUS "core sub dir names:${subdir_names}")

if (NOT HJ_ENABLE_RENDER_PRIO)
    list(FILTER subdir_names EXCLUDE REGEX "^prio$")
endif()

HJ_RETRIEVE_XFILES(SUBPROJ_SOURCE_FILES ${subdir_names})
HJ_RETRIEVE_HFILES(SUBPROJ_HEADER_FILES ${subdir_names})

file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cc" "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
list(APPEND HEADER_FILES ${SUBPROJ_HEADER_FILES})
list(APPEND SOURCE_FILES ${SUBPROJ_SOURCE_FILES})

# list(APPEND SOURCE_FILES ${CMAKE_SOURCE_DIR}/Dependencies/generic/spdlog/details/registry-inl.cc)

MAKE_COMMON_CONFIG()

add_library(${PROJ_NAME} STATIC ${SOURCE_FILES} ${HEADER_FILES})

target_link_libraries(${PROJ_NAME} PRIVATE HJConvert HJStat LIBSPDLOG LIBYUV LIBSTB HJUtils yyjson LIBREFLCPP)

if (HJ_ENABLE_RENDER_PRIO)
    target_compile_definitions(${PROJ_NAME} PRIVATE HJ_ENABLE_RENDER_PRIO)
endif()

if (WINDOWS OR MACOSX)
    target_link_libraries(${PROJ_NAME} PRIVATE glfw)
elseif(ANDROID)
    target_link_libraries(${PROJ_NAME} PRIVATE EGL GLESv3)
endif()

target_include_directories(${PROJ_NAME} INTERFACE ${${PROJ_NAME}_INCLUDE_DIRS})

HJ_CONFIG_FRAMEWORK(${PROJ_NAME})

set_target_properties(${PROJ_NAME} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${ARCHIVE_output}
    LIBRARY_OUTPUT_DIRECTORY ${library_output}
    RUNTIME_OUTPUT_DIRECTORY ${runtime_output})
	
ProjFolder(${PROJ_NAME} "src")
