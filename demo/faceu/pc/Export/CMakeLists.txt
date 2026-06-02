cmake_minimum_required(VERSION 3.15)

project(HJFaceuExportDemo LANGUAGES CXX)

if(CMAKE_GENERATOR_PLATFORM STREQUAL "Win32")
   set(ARCHS_NAME "x86")
elseif(CMAKE_GENERATOR_PLATFORM MATCHES "^(x64|AMD64)$")
   set(ARCHS_NAME "x64")
elseif(CMAKE_GENERATOR_PLATFORM MATCHES "ARM64")
   set(ARCHS_NAME "arm64-v8a")
elseif(CMAKE_GENERATOR_PLATFORM MATCHES "")    ### optional platform for generator (if empty, generator use x64)                                
   set(ARCHS_NAME "x64")      
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB EXPORT_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

set(EXPORT_CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/core)
set(EXPORT_DEP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/dep)
set(IMGUI_DEP_DIR ${EXPORT_DEP_DIR}/imgui)
set(GLFW_DEP_DIR ${EXPORT_DEP_DIR}/glfw)
set(EXPORT_RESOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/resource)

add_executable(${PROJECT_NAME} ${EXPORT_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${EXPORT_CORE_DIR}/include
    ${IMGUI_DEP_DIR}/include
    ${IMGUI_DEP_DIR}/include/backends
    ${GLFW_DEP_DIR}/include
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    EXPORT_RESOURCE_ROOT="${EXPORT_RESOURCE_DIR}"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${EXPORT_CORE_DIR}/lib/${ARCHS_NAME}/HJRender.lib
    ${IMGUI_DEP_DIR}/lib/${ARCHS_NAME}/imgui.lib
    ${GLFW_DEP_DIR}/lib/${ARCHS_NAME}/glfw3.lib
)

if (WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE winmm.lib Crypt32.lib)
endif()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${EXPORT_CORE_DIR}/lib/${ARCHS_NAME}/HJRender.dll
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMENT "Copy HJRender.dll to output directory"
)
