set(PROJ_NAME HJMediaUI)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${HJ_ROOT_PATH}/src/entry/inference)
include_directories(${HJ_ROOT_PATH}/src/entry/inference/utils)
include_directories(${HJ_ROOT_PATH}/src/convert/utils)
include_directories(${HJ_ROOT_PATH}/src/utils)
include_directories(${HJ_ROOT_PATH}/src/utils/base)

file(GLOB UI_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB UI_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.m"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")


set(APP_RESOURCE_DIR ${HJ_ROOT_PATH}/examples/resource)
if (NOT IS_DIRECTORY ${APP_RESOURCE_DIR})
    message(FATAL_ERROR "Resource directory ${APP_RESOURCE_DIR} not found")
endif()
file(GLOB_RECURSE HJMEDIA_APP_RESOURCES
    CONFIGURE_DEPENDS
    LIST_DIRECTORIES FALSE
    ${APP_RESOURCE_DIR}/*)

foreach(_resource_file ${HJMEDIA_APP_RESOURCES})
    file(RELATIVE_PATH _resource_relative_path ${APP_RESOURCE_DIR} ${_resource_file})
    get_filename_component(_resource_relative_dir ${_resource_relative_path} DIRECTORY)
    set(_resource_destination "Resources/resource")
    if (NOT (_resource_relative_dir STREQUAL "" OR _resource_relative_dir STREQUAL "."))
        string(REPLACE "\\" "/" _resource_relative_dir ${_resource_relative_dir})
        set(_resource_destination "${_resource_destination}/${_resource_relative_dir}")
    endif()
    set_source_files_properties(${_resource_file} PROPERTIES MACOSX_PACKAGE_LOCATION ${_resource_destination})
endforeach()

set(INFO_PLIST ${CMAKE_CURRENT_BINARY_DIR}/Info.plist)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in ${INFO_PLIST} @ONLY)

add_executable(${PROJ_NAME} MACOSX_BUNDLE
    ${UI_SOURCES}
    ${UI_HEADERS}
    ${HJMEDIA_APP_RESOURCES}
)

target_include_directories(${PROJ_NAME}
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${HJ_ROOT_PATH}/src/entry/inference"
        "${HJ_ROOT_PATH}/src/entry/inference/utils"
        "${HJ_ROOT_PATH}/src/convert/utils"
        "${HJ_ROOT_PATH}/src/utils"
        "${HJ_ROOT_PATH}/src/utils/base"
)

set_target_properties(${PROJ_NAME} PROPERTIES LINKER_LANGUAGE CXX)

set_target_properties(${PROJ_NAME} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${ARCHIVE_output}
    LIBRARY_OUTPUT_DIRECTORY ${library_output}
    RUNTIME_OUTPUT_DIRECTORY ${runtime_output})


MAKE_COMMON_CONFIG(${UI_SOURCES})

find_library(UIKIT_FRAMEWORK UIKit)
find_library(QUARTZCORE_FRAMEWORK QuartzCore)
find_library(OPENGLES_FRAMEWORK OpenGLES)
find_library(COREGRAPHICS_FRAMEWORK CoreGraphics)
find_library(COREIMAGE_FRAMEWORK CoreImage)
find_library(COREML_FRAMEWORK CoreML)
find_library(COREVIDEO_FRAMEWORK CoreVideo)
find_library(COREMEDIA_FRAMEWORK CoreMedia)
find_library(VIDEOTOOLBOX_FRAMEWORK VideoToolbox)

target_link_libraries(${PROJ_NAME}
    PRIVATE
        HJRender
        HJInference
        ${UIKIT_FRAMEWORK}
        ${QUARTZCORE_FRAMEWORK}
        ${OPENGLES_FRAMEWORK}
        ${COREGRAPHICS_FRAMEWORK}
        ${COREIMAGE_FRAMEWORK}
        ${COREML_FRAMEWORK}
        ${COREVIDEO_FRAMEWORK}
        ${COREMEDIA_FRAMEWORK}
        ${VIDEOTOOLBOX_FRAMEWORK}
)

set_target_properties(${PROJ_NAME} PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST ${INFO_PLIST}
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.hjmedia.ui"
    XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
    XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/../Frameworks @loader_path/Frameworks"
)

add_dependencies(${PROJ_NAME} HJRender HJInference)
set(_app_frameworks_dir "$<TARGET_BUNDLE_DIR:${PROJ_NAME}>/Frameworks")
set(_moltenvk_framework_src "${HJ_ROOT_PATH}/externals/ios/ncnn/MoltenVK.framework")
set(_moltenvk_binary_src "${_moltenvk_framework_src}/MoltenVK")
set(_embed_moltenvk_script "${HJ_ROOT_PATH}/cmake/Modules/EmbedMoltenVKiOS.cmake")
add_custom_command(TARGET ${PROJ_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${_app_frameworks_dir}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "$<TARGET_FILE_DIR:HJRender>" "${_app_frameworks_dir}/HJRender.framework"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "$<TARGET_FILE_DIR:HJInference>" "${_app_frameworks_dir}/HJInference.framework"
    COMMAND ${CMAKE_COMMAND}
        "-DEMBED_APP_FRAMEWORKS_DIR:PATH=${_app_frameworks_dir}"
        "-DEMBED_MOLTENVK_FRAMEWORK_SRC:PATH=${_moltenvk_framework_src}"
        "-DEMBED_MOLTENVK_BINARY_SRC:PATH=${_moltenvk_binary_src}"
        -P "${_embed_moltenvk_script}"
    COMMENT "Embedding HJRender.framework into app bundle")

set(_faceu_demo_root "${HJ_ROOT_PATH}/demo/faceu/ios/Export")
set(_faceu_demo_core_dir "${_faceu_demo_root}/core")
set(_faceu_demo_source_dir "${_faceu_demo_root}/demo")
set(_faceu_demo_resource_dir "${_faceu_demo_root}/resource")
if(CMAKE_CONFIGURATION_TYPES)
    set(_faceu_export_guard "$<CONFIG:Release>")
    set(_faceu_active_config "$<CONFIG>")
else()
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(_faceu_export_guard "ON")
    else()
        set(_faceu_export_guard "")
    endif()
    set(_faceu_active_config "${CMAKE_BUILD_TYPE}")
endif()
set(_faceu_export_script "${HJ_ROOT_PATH}/cmake/Modules/FaceuIOSExport.cmake")
add_custom_command(TARGET ${PROJ_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND}
        "-DFACEU_IS_RELEASE:BOOL=${_faceu_export_guard}"
        "-DFACEU_ACTIVE_CONFIG:STRING=${_faceu_active_config}"
        "-DFACEU_FRAMEWORK_SRC:PATH=$<TARGET_FILE_DIR:HJRender>"
        "-DFACEU_DEMO_CORE_DIR:PATH=${_faceu_demo_core_dir}"
        "-DFACEU_DEMO_SOURCE_DIR:PATH=${_faceu_demo_source_dir}"
        "-DFACEU_DEMO_RESOURCE_DIR:PATH=${_faceu_demo_resource_dir}"
        "-DFACEU_APP_RESOURCE_DIR:PATH=${APP_RESOURCE_DIR}"
        "-DFACEU_SOURCE_DIR:PATH=${CMAKE_CURRENT_SOURCE_DIR}"
        -P "${_faceu_export_script}"
    COMMENT "Preparing demo/faceu/ios package with framework, demo sources, and resources (Release-only)")
