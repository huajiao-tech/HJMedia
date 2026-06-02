set(PROJ_NAME HJMediaTest)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/third_party/glfw/deps)

file(GLOB CUR_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB CUR_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

list(APPEND HEADER_FILES ${CUR_HEADER_FILES})
list(APPEND SOURCE_FILES ${CUR_SOURCE_FILES})

#set(COMMON_LIBS LIBGTEST HJGraphs HJPlugins HJStat HJComp HJMedia HJPlatform HJUtils LIBFFMPEG LIBSPDLOG LIBYUV)

set(COMMON_LIBS LIBGTEST HJComp HJUtils LIBSPDLOG LIBYUV)

if(WINDOWS)
    list(APPEND SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/item/HJRenderGraph_test.cpp")
endif()

add_executable(${PROJ_NAME} ${SOURCE_FILES})

target_link_libraries(${PROJ_NAME} PRIVATE ${COMMON_LIBS})

if (WINDOWS)  
    target_link_libraries(${PROJ_NAME} PRIVATE implot imgui glfw)
    target_link_libraries(${PROJ_NAME} PRIVATE winmm.lib Crypt32.lib)
endif()

set_target_properties(${PROJ_NAME} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${ARCHIVE_output}
    LIBRARY_OUTPUT_DIRECTORY ${library_output}
    RUNTIME_OUTPUT_DIRECTORY ${runtime_output})
ProjFolder(${PROJ_NAME} "MainUI")
