set(PROJ_NAME HJFaceuUI)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# message (STATUS "retrieve_files entry")
HJ_SUBDIR_LIST(subdir_names ${CMAKE_CURRENT_SOURCE_DIR})
# message (STATUS "core sub dir names:${subdir_names}")

HJ_RETRIEVE_XFILES(SUBPROJ_SOURCE_FILES ${subdir_names})
HJ_RETRIEVE_HFILES(SUBPROJ_HEADER_FILES ${subdir_names})

file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cc" "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
list(APPEND HEADER_FILES ${SUBPROJ_HEADER_FILES})
list(APPEND SOURCE_FILES ${SUBPROJ_SOURCE_FILES})

# file(GLOB UI_UTILS_SOURCES "${UI_ROOT}/utils/*.cpp" "${UI_ROOT}/utils/*.cc" "${UI_ROOT}/Fonts/*.cpp")
# list(APPEND SOURCE_FILES ${UI_UTILS_SOURCES})

# list(APPEND SOURCE_FILES "${UI_ROOT}/App.cpp")
# list(APPEND SOURCE_FILES "${UI_ROOT}/demo.cpp")

# list(APPEND SOURCE_FILES "${UI_ROOT}/item/HJUIBaseItem.cpp")
# list(APPEND SOURCE_FILES "${UI_ROOT}/item/HJUIItemFaceuTool.cpp")

MAKE_COMMON_CONFIG()

add_executable(${PROJ_NAME} ${SOURCE_FILES} ${HEADER_FILES})

#target_link_libraries(${PROJ_NAME} PRIVATE HJStat HJComp HJUtils LIBSPDLOG LIBYUV LIBSTB implot imgui glfw)
target_link_libraries(${PROJ_NAME} PRIVATE HJRender HJConvert HJStat HJComp HJUtils LIBSPDLOG LIBYUV LIBSTB implot imgui glfw yyjson LIBREFLCPP)

if (WINDOWS)
    target_link_libraries(${PROJ_NAME} PRIVATE winmm.lib Crypt32.lib)
endif()    
#target_include_directories(${PROJ_NAME} INTERFACE ${${PROJ_NAME}_INCLUDE_DIRS})

HJ_CONFIG_FRAMEWORK(${PROJ_NAME})

set_target_properties(${PROJ_NAME} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${ARCHIVE_output}
    LIBRARY_OUTPUT_DIRECTORY ${library_output}
    RUNTIME_OUTPUT_DIRECTORY ${runtime_output})

set(EXPORT_DEP_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../Export/dep)
set(IMGUI_EXPORT_DIR ${EXPORT_DEP_ROOT}/imgui)
set(IMGUI_EXPORT_INCLUDE ${IMGUI_EXPORT_DIR}/include)
set(GLFW_EXPORT_DIR ${EXPORT_DEP_ROOT}/glfw)
set(GLFW_EXPORT_INCLUDE ${GLFW_EXPORT_DIR}/include)
set(EXPORT_CORE_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../Export/core)
set(CORE_EXPORT_INCLUDE ${EXPORT_CORE_ROOT}/include)
set(GLFW_EXPORT_LIB_DIR ${GLFW_EXPORT_DIR}/lib/${ARCHS_NAME})
set(IMGUI_EXPORT_LIB_DIR ${IMGUI_EXPORT_DIR}/lib/${ARCHS_NAME})
set(CORE_EXPORT_LIB_DIR ${EXPORT_CORE_ROOT}/lib/${ARCHS_NAME})

set(IMGUI_ROOT_HEADERS
    ${HJ_THIRDPARTY_PATH}/imgui/imconfig.h
    ${HJ_THIRDPARTY_PATH}/imgui/imgui_internal.h
    ${HJ_THIRDPARTY_PATH}/imgui/imgui.h
    ${HJ_THIRDPARTY_PATH}/imgui/imstb_rectpack.h
    ${HJ_THIRDPARTY_PATH}/imgui/imstb_textedit.h
    ${HJ_THIRDPARTY_PATH}/imgui/imstb_truetype.h
)
set(IMGUI_BACKEND_HEADERS
    ${HJ_THIRDPARTY_PATH}/imgui/backends/imgui_impl_glfw.h
    ${HJ_THIRDPARTY_PATH}/imgui/backends/imgui_impl_opengl3.h
    ${HJ_THIRDPARTY_PATH}/imgui/backends/imgui_impl_opengl3_loader.h
)
set(IMGUI_MISC_HEADERS
    ${HJ_THIRDPARTY_PATH}/imgui/misc/cpp/imgui_stdlib.h
)
set(HJRENDER_EXPORT_HEADERS
    ${CMAKE_SOURCE_DIR}/src/entry/render/HJRenderContextExport.h
    ${CMAKE_SOURCE_DIR}/src/entry/render/HJRenderFaceuExport.h
    ${CMAKE_SOURCE_DIR}/src/entry/HJCommonInterface.h
    ${CMAKE_SOURCE_DIR}/src/utils/base/HJExport.h
)

ProjFolder(${PROJ_NAME} "Demo")

# Copy resource directory to output directory
copy_directory_post_build(${PROJ_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/../../../../examples/resource resource)

add_custom_command(TARGET ${PROJ_NAME} POST_BUILD
    COMMAND $<$<CONFIG:Release>:${CMAKE_COMMAND}> -E make_directory
        ${GLFW_EXPORT_DIR}
        ${GLFW_EXPORT_INCLUDE}
        ${IMGUI_EXPORT_DIR}
        ${IMGUI_EXPORT_INCLUDE}
        ${IMGUI_EXPORT_INCLUDE}/backends
        ${IMGUI_EXPORT_INCLUDE}/misc/cpp
        ${EXPORT_CORE_ROOT}
        ${CORE_EXPORT_INCLUDE}
        ${GLFW_EXPORT_LIB_DIR}
        ${IMGUI_EXPORT_LIB_DIR}
        ${CORE_EXPORT_LIB_DIR}
    COMMAND $<$<CONFIG:Release>:${CMAKE_COMMAND}> -E copy_if_different
        $<TARGET_FILE:glfw>
        ${GLFW_EXPORT_LIB_DIR}
    COMMAND $<$<CONFIG:Release>:${CMAKE_COMMAND}> -E copy_if_different
        $<TARGET_FILE:imgui>
        ${IMGUI_EXPORT_LIB_DIR}
    COMMAND $<$<CONFIG:Release>:${CMAKE_COMMAND}> -E copy_directory
        ${HJ_THIRDPARTY_PATH}/glfw/include
        ${GLFW_EXPORT_INCLUDE}
    COMMAND $<$<CONFIG:Release>:${CMAKE_COMMAND}> -E copy_directory
        ${HJ_THIRDPARTY_PATH}/glfw/deps
        ${GLFW_EXPORT_INCLUDE}
    COMMAND $<$<CONFIG:Release>:${CMAKE_COMMAND}> -E copy_if_different
        ${IMGUI_ROOT_HEADERS}
        ${IMGUI_EXPORT_INCLUDE}
    COMMAND $<$<CONFIG:Release>:${CMAKE_COMMAND}> -E copy_if_different
        ${IMGUI_BACKEND_HEADERS}
        ${IMGUI_EXPORT_INCLUDE}/backends
    COMMAND $<$<CONFIG:Release>:${CMAKE_COMMAND}> -E copy_if_different
        ${IMGUI_MISC_HEADERS}
        ${IMGUI_EXPORT_INCLUDE}/misc/cpp
    COMMAND $<$<CONFIG:Release>:${CMAKE_COMMAND}> -E copy_if_different
        $<TARGET_FILE:HJRender>
        ${CORE_EXPORT_LIB_DIR}
    COMMAND $<$<CONFIG:Release>:${CMAKE_COMMAND}> -E copy_if_different
        $<TARGET_LINKER_FILE:HJRender>
        ${CORE_EXPORT_LIB_DIR}
    COMMAND $<$<CONFIG:Release>:${CMAKE_COMMAND}> -E copy_if_different   
        ${HJRENDER_EXPORT_HEADERS}
        ${CORE_EXPORT_INCLUDE}
    COMMENT "Exporting imgui/glfw headers and libs to ${EXPORT_DEP_ROOT} (Release only)"
)

add_custom_command(TARGET ${PROJ_NAME} POST_BUILD
    COMMAND $<$<CONFIG:Release>:${CMAKE_COMMAND}> -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../examples/resource
        ${CMAKE_CURRENT_SOURCE_DIR}/../Export/resource
    COMMENT "Copy examples/resource to Export/resource"
)

message (STATUS "PROJ_NAME: ${PROJ_NAME}")
