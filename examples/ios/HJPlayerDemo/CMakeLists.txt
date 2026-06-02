set(PROJ_NAME HJPlayerDemo)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB UI_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB UI_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.m"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.mm")

if (UI_SOURCES STREQUAL "")
    set(PLACEHOLDER_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/HJPlayerDemoPlaceholder.mm")
    file(WRITE "${PLACEHOLDER_SOURCE}" "int main(int argc, char* argv[]) { (void)argc; (void)argv; return 0; }\n")
    list(APPEND UI_SOURCES "${PLACEHOLDER_SOURCE}")
endif()

set(INFO_PLIST_TEMPLATE "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in")
if (EXISTS "${INFO_PLIST_TEMPLATE}")
    set(INFO_PLIST ${CMAKE_CURRENT_BINARY_DIR}/Info.plist)
    configure_file("${INFO_PLIST_TEMPLATE}" "${INFO_PLIST}" @ONLY)
else()
    set(INFO_PLIST "${HJ_ROOT_PATH}/cmake/isys/Info.plist")
endif()

set(DEMO_RESOURCE "${CMAKE_CURRENT_SOURCE_DIR}/res/c58733ac51124fe38cdc6540a7b8fa46.mkv")
if (EXISTS "${DEMO_RESOURCE}")
    set_source_files_properties("${DEMO_RESOURCE}" PROPERTIES MACOSX_PACKAGE_LOCATION Resources/res)
endif()

set(DEMO_APP_SOURCES
    ${UI_SOURCES}
    ${UI_HEADERS}
)
if (EXISTS "${DEMO_RESOURCE}")
    list(APPEND DEMO_APP_SOURCES "${DEMO_RESOURCE}")
endif()

add_executable(${PROJ_NAME} MACOSX_BUNDLE
    ${DEMO_APP_SOURCES}
)

target_include_directories(${PROJ_NAME}
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${HJ_ROOT_PATH}/src/entry/player/isys"
)

set_target_properties(${PROJ_NAME} PROPERTIES
    LINKER_LANGUAGE OBJCXX
    ARCHIVE_OUTPUT_DIRECTORY ${ARCHIVE_output}
    LIBRARY_OUTPUT_DIRECTORY ${library_output}
    RUNTIME_OUTPUT_DIRECTORY ${runtime_output}
    MACOSX_BUNDLE_INFO_PLIST "${INFO_PLIST}"
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.hjmedia.playerdemo"
    XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
    XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/../Frameworks @loader_path/Frameworks"
)

MAKE_COMMON_CONFIG(${UI_SOURCES})

find_library(UIKIT_FRAMEWORK UIKit)
find_library(FOUNDATION_FRAMEWORK Foundation)

target_link_libraries(${PROJ_NAME}
    PRIVATE
        HJMediaPlayer
        ${UIKIT_FRAMEWORK}
        ${FOUNDATION_FRAMEWORK}
)

add_dependencies(${PROJ_NAME} HJMediaPlayer)
set(_app_frameworks_dir "$<TARGET_BUNDLE_DIR:${PROJ_NAME}>/Frameworks")
add_custom_command(TARGET ${PROJ_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${_app_frameworks_dir}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "$<TARGET_FILE_DIR:HJMediaPlayer>" "${_app_frameworks_dir}/HJMediaPlayer.framework"
    COMMENT "Embedding HJMediaPlayer.framework into app bundle")
