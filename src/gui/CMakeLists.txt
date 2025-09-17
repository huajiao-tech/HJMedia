set(PROJ_NAME HJGui)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/Dependencies/generic)
include_directories(${CMAKE_SOURCE_DIR}/src/utils)
include_directories(${CMAKE_SOURCE_DIR}/src/media)
include_directories(${CMAKE_SOURCE_DIR}/src/media/codec)
include_directories(${CMAKE_SOURCE_DIR}/src/media/muxer)
include_directories(${CMAKE_SOURCE_DIR}/src/media/demuxer)
include_directories(${CMAKE_SOURCE_DIR}/src/media/render)
include_directories(${CMAKE_SOURCE_DIR}/src/gui)
include_directories(${CMAKE_SOURCE_DIR}/src/gui/imgui)

include_directories(${HJ_EXTERNALS_PATH}/generic)
include_directories(${HJ_EXTERNALS_PATH}/generic/ffmpeg)

file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cc" "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
# imgui core
# file(GLOB IMGUI_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/imgui/*.h")
# file(GLOB IMGUI_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/imgui/*.cc" "${CMAKE_CURRENT_SOURCE_DIR}/imgui/*.cpp")
# # backends
# list(APPEND IMGUI_HEADER_FILES 
#     "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_glfw.h"
#     "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_opengl3.h" 
#     "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_opengl3_loader.h"
#     "${CMAKE_CURRENT_SOURCE_DIR}/imgui/ImGuiFileDialog/ImGuiFileDialog.h"
#     "${CMAKE_CURRENT_SOURCE_DIR}/imgui/ImGuiFileDialog/ImGuiFileDialogConfig.h"
#     "${CMAKE_CURRENT_SOURCE_DIR}/imgui/ImFileDialog/ImFileDialog.h"
#     "${CMAKE_CURRENT_SOURCE_DIR}/imgui/implot/implot.h"
#     "${CMAKE_CURRENT_SOURCE_DIR}/imgui/implot/implot_internal.h"
#     )
# list(APPEND IMGUI_SOURCE_FILES 
#     "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_glfw.cpp"
#     "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_opengl3.cpp"
#     "${CMAKE_CURRENT_SOURCE_DIR}/imgui/ImGuiFileDialog/ImGuiFileDialog.cpp"
#     "${CMAKE_CURRENT_SOURCE_DIR}/imgui/ImFileDialog/ImFileDialog.cpp"
#     "${CMAKE_CURRENT_SOURCE_DIR}/imgui/implot/implot.cpp"
#     "${CMAKE_CURRENT_SOURCE_DIR}/imgui/implot/implot_items.cpp"
#     )
# list(APPEND HEADER_FILES ${IMGUI_HEADER_FILES})
# list(APPEND SOURCE_FILES ${IMGUI_SOURCE_FILES})

# if(WINDOWS OR ANDROID) 
# list(APPEND HEADER_FILES 
#     "${CMAKE_CURRENT_SOURCE_DIR}/backends/imgui_impl_vulkan.h"
#     )
# list(APPEND SOURCE_FILES 
#     "${CMAKE_CURRENT_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp"
#     )
# elseif(APPLE)
# list(APPEND HEADER_FILES 
#     "${CMAKE_CURRENT_SOURCE_DIR}/backends/imgui_impl_metal.h"
#     )
# list(APPEND SOURCE_FILES 
#     "${CMAKE_CURRENT_SOURCE_DIR}/backends/imgui_impl_metal.mm"
#     )
# endif() #GUI_VULKAN

set(HJGUI_INCLUDE_DIRS 
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui)

MAKE_COMMON_CONFIG()

add_library(${PROJ_NAME} STATIC ${SOURCE_FILES} ${HEADER_FILES})
target_link_libraries(${PROJ_NAME} PRIVATE HJCore HJMedia HJUtils LIBFFMPEG ImGuiFileDialog ImFileDialog implot imgui LIBSTB LIBSPDLOG LIBGLFW)
target_include_directories(${PROJ_NAME} INTERFACE ${HJGUI_INCLUDE_DIRS})

HJ_CONFIG_FRAMEWORK(${PROJ_NAME})

set_target_properties(${PROJ_NAME} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${ARCHIVE_output}
    LIBRARY_OUTPUT_DIRECTORY ${library_output}
    RUNTIME_OUTPUT_DIRECTORY ${runtime_output})
	
ProjFolder(${PROJ_NAME} "src")