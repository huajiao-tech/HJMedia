cmake_minimum_required(VERSION 3.15)

project(HJFaceuExport VERSION 1.0 LANGUAGES C CXX OBJC OBJCXX)

set(DEMO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/demo")
set(RESOURCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/resource")

if (NOT IS_DIRECTORY "${DEMO_ROOT}")
    message(FATAL_ERROR "Demo sources were not found under ${DEMO_ROOT}. Run the top-level build to generate the export package first.")
endif()

if (NOT IS_DIRECTORY "${RESOURCE_ROOT}")
    message(FATAL_ERROR "Resource directory ${RESOURCE_ROOT} was not generated. Run the top-level build first.")
endif()

set(_hjrender_search_path "${CMAKE_CURRENT_SOURCE_DIR}/core")
find_library(HJRENDER_FRAMEWORK
    NAMES HJRender
    PATHS "${_hjrender_search_path}"
    NO_DEFAULT_PATH
)

if (NOT HJRENDER_FRAMEWORK)
    message(FATAL_ERROR "HJRender.framework missing under ${_hjrender_search_path}. Build the SDK to generate it before configuring this demo.")
endif()

set(HJRENDER_FRAMEWORK_ROOT "${HJRENDER_FRAMEWORK}")


set(PROJ_NAME "${PROJECT_NAME}")

file(GLOB UI_HEADERS
    "${DEMO_ROOT}/*.h"
)

file(GLOB UI_SOURCES
    "${DEMO_ROOT}/*.m"
    "${DEMO_ROOT}/*.mm"
    "${DEMO_ROOT}/*.cc"
    "${DEMO_ROOT}/*.cpp"
)

set(INFO_PLIST "${CMAKE_CURRENT_BINARY_DIR}/Info.plist")
configure_file("${DEMO_ROOT}/../Info.plist.in" "${INFO_PLIST}" @ONLY)

file(GLOB_RECURSE DEMO_APP_RESOURCES
    LIST_DIRECTORIES FALSE
    "${RESOURCE_ROOT}/*"
)

foreach(_resource_file ${DEMO_APP_RESOURCES})
    file(RELATIVE_PATH _resource_relative_path "${RESOURCE_ROOT}" "${_resource_file}")
    get_filename_component(_resource_relative_dir "${_resource_relative_path}" DIRECTORY)
    set(_resource_destination "Resources/resource")
    if (NOT (_resource_relative_dir STREQUAL "" OR _resource_relative_dir STREQUAL "."))
        string(REPLACE "\\" "/" _resource_relative_dir "${_resource_relative_dir}")
        set(_resource_destination "${_resource_destination}/${_resource_relative_dir}")
    endif()
    set_source_files_properties("${_resource_file}" PROPERTIES MACOSX_PACKAGE_LOCATION "${_resource_destination}")
endforeach()

add_executable(${PROJECT_NAME} MACOSX_BUNDLE
    ${UI_SOURCES}
    ${UI_HEADERS}
    ${DEMO_APP_RESOURCES}
)

set_source_files_properties(${UI_SOURCES} PROPERTIES COMPILE_FLAGS "-x objective-c++ -fobjc-arc")

target_include_directories(${PROJECT_NAME}
    PRIVATE
        "${DEMO_ROOT}"
        "${HJRENDER_FRAMEWORK_ROOT}/Headers"
)

find_library(UIKIT_FRAMEWORK UIKit)
find_library(QUARTZCORE_FRAMEWORK QuartzCore)
find_library(OPENGLES_FRAMEWORK OpenGLES)
find_library(COREGRAPHICS_FRAMEWORK CoreGraphics)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        ${HJRENDER_FRAMEWORK}
        ${UIKIT_FRAMEWORK}
        ${QUARTZCORE_FRAMEWORK}
        ${OPENGLES_FRAMEWORK}
        ${COREGRAPHICS_FRAMEWORK}
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST "${INFO_PLIST}"
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.hjmedia.faceu"
    XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
    XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "13.0"
    XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/../Frameworks @loader_path/Frameworks"
    XCODE_ATTRIBUTE_FRAMEWORK_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/core"
)

set(HJ_OUT_PATH ${PROJECT_BINARY_DIR}/out/libs)
set(library_output ${HJ_OUT_PATH})
set(ARCHIVE_output ${HJ_OUT_PATH})
set(runtime_output ${HJ_OUT_PATH})
set_target_properties(${PROJECT_NAME} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${ARCHIVE_output}
    LIBRARY_OUTPUT_DIRECTORY ${library_output}
    RUNTIME_OUTPUT_DIRECTORY ${runtime_output})




set(_app_frameworks_dir "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/Frameworks")
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${_app_frameworks_dir}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${HJRENDER_FRAMEWORK_ROOT}" "${_app_frameworks_dir}/HJRender.framework"
    COMMENT "Embedding packaged HJRender.framework into app bundle"
)
