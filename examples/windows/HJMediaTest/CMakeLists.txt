set(PROJ_NAME HJMediaTest)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/item)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/utils)
include_directories(${HJ_EXTERNALS_PATH}/generic/ffmpeg)
#set(${PROJ_NAME}_INCLUDE_DIRS 
#    ${CMAKE_CURRENT_SOURCE_DIR})

#if (HarmonyOS)
#    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/hsys)
#    list(APPEND ${PROJ_NAME}_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/hsys)
#endif()

# message (STATUS "retrieve_files entry")
HJ_SUBDIR_LIST(subdir_names ${CMAKE_CURRENT_SOURCE_DIR})
# message (STATUS "core sub dir names:${subdir_names}")

HJ_RETRIEVE_XFILES(SUBPROJ_SOURCE_FILES ${subdir_names})
HJ_RETRIEVE_HFILES(SUBPROJ_HEADER_FILES ${subdir_names})

MAKE_COMMON_CONFIG()

set(COMMON_LIBS HJGraphs HJPlugins HJStat HJComp HJMedia HJPlatform HJUtils LIBFFMPEG LIBSPDLOG LIBYUV implot imgui glfw winmm.lib Crypt32.lib)

# --- Test for HJThreadPool ---
set(PROJ_NAME_THREAD_POOL HJThreadPool_test)
add_executable(${PROJ_NAME_THREAD_POOL} ${PROJ_NAME_THREAD_POOL}.cpp)
target_link_libraries(${PROJ_NAME_THREAD_POOL} PRIVATE ${COMMON_LIBS})
HJ_CONFIG_FRAMEWORK(${PROJ_NAME_THREAD_POOL})
set_target_properties(${PROJ_NAME_THREAD_POOL} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${ARCHIVE_output}
    LIBRARY_OUTPUT_DIRECTORY ${library_output}
    RUNTIME_OUTPUT_DIRECTORY ${runtime_output})
ProjFolder(${PROJ_NAME_THREAD_POOL} "MainUI")


# --- To add a new test, for example New_test.cpp, add the following block ---
# set(PROJ_NAME_NEW_TEST New_test)
# add_executable(${PROJ_NAME_NEW_TEST} New_test.cpp)
# target_link_libraries(${PROJ_NAME_NEW_TEST} PRIVATE ${COMMON_LIBS})
# HJ_CONFIG_FRAMEWORK(${PROJ_NAME_NEW_TEST})
# set_target_properties(${PROJ_NAME_NEW_TEST} PROPERTIES
#     ARCHIVE_OUTPUT_DIRECTORY ${ARCHIVE_output}
#     LIBRARY_OUTPUT_DIRECTORY ${library_output}
#     RUNTIME_OUTPUT_DIRECTORY ${runtime_output})
# ProjFolder(${PROJ_NAME_NEW_TEST} "MainUI")