set(PROJ_NAME HJMediaGraphicUI)

set(UI_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../HJMediaUI)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

include_directories(${UI_ROOT})
include_directories(${UI_ROOT}/item)
include_directories(${UI_ROOT}/utils)
include_directories(${UI_ROOT}/Fonts)
include_directories(${UI_ROOT}/include)

add_definitions(-DHJ_MEDIA_GRAPHIC_UI)

# message (STATUS "retrieve_files entry")
HJ_SUBDIR_LIST(subdir_names ${CMAKE_CURRENT_SOURCE_DIR})
# message (STATUS "core sub dir names:${subdir_names}")

HJ_RETRIEVE_XFILES(SUBPROJ_SOURCE_FILES ${subdir_names})
HJ_RETRIEVE_HFILES(SUBPROJ_HEADER_FILES ${subdir_names})

file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cc" "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
list(APPEND HEADER_FILES ${SUBPROJ_HEADER_FILES})
list(APPEND SOURCE_FILES ${SUBPROJ_SOURCE_FILES})

file(GLOB UI_UTILS_SOURCES "${UI_ROOT}/utils/*.cpp" "${UI_ROOT}/utils/*.cc" "${UI_ROOT}/Fonts/*.cpp")
list(REMOVE_ITEM UI_UTILS_SOURCES "${UI_ROOT}/utils/HJNodePipelineEditor.cpp")
list(APPEND SOURCE_FILES ${UI_UTILS_SOURCES})

list(APPEND SOURCE_FILES "${UI_ROOT}/App.cpp")
list(APPEND SOURCE_FILES "${UI_ROOT}/demo.cpp")

list(APPEND SOURCE_FILES "${UI_ROOT}/item/HJUIBaseItem.cpp")
list(APPEND SOURCE_FILES "${UI_ROOT}/item/HJUIItemFaceuTool.cpp")

MAKE_COMMON_CONFIG()

add_executable(${PROJ_NAME} ${SOURCE_FILES} ${HEADER_FILES})

target_link_libraries(${PROJ_NAME} PRIVATE HJStat HJComp HJConvert HJUtils LIBSPDLOG LIBYUV LIBSTB implot imgui glfw yyjson LIBREFLCPP)

if (WINDOWS)
    target_link_libraries(${PROJ_NAME} PRIVATE winmm.lib Crypt32.lib)
endif()    
#target_include_directories(${PROJ_NAME} INTERFACE ${${PROJ_NAME}_INCLUDE_DIRS})

HJ_CONFIG_FRAMEWORK(${PROJ_NAME})

set_target_properties(${PROJ_NAME} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${ARCHIVE_output}
    LIBRARY_OUTPUT_DIRECTORY ${library_output}
    RUNTIME_OUTPUT_DIRECTORY ${runtime_output})

ProjFolder(${PROJ_NAME} "MainUI")

# Copy resource directory to output directory
copy_directory_post_build(${PROJ_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/../../resource resource)

message (STATUS "PROJ_NAME: ${PROJ_NAME}")
