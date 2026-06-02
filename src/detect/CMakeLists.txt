set(PROJ_NAME HJDetect)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

if (HJ_ENABLE_TNN)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/tnn)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/tnn/utils)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/tnn/base)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/ncnn) 
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/ncnn/utils) 

if (HarmonyOS)
	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/hsys/mindspore)
endif()
if (IOS)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/osys/coreml)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/osys/vision)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/osys/VTFrameProcessor)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/utils) 
   
#add_definitions(-DBUILDING_DLL)

set(${PROJ_NAME}_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR})
list(APPEND ${PROJ_NAME}_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/ncnn)  # export another use
if (HJ_ENABLE_TNN)
    list(APPEND ${PROJ_NAME}_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/tnn)
endif()

# message (STATUS "retrieve_files entry")
HJ_SUBDIR_LIST(subdir_names ${CMAKE_CURRENT_SOURCE_DIR})
# message (STATUS "core sub dir names:${subdir_names}")

HJ_RETRIEVE_XFILES(SUBPROJ_SOURCE_FILES ${subdir_names})
HJ_RETRIEVE_HFILES(SUBPROJ_HEADER_FILES ${subdir_names})

file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cc" "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
	
list(APPEND HEADER_FILES ${SUBPROJ_HEADER_FILES})
list(APPEND SOURCE_FILES ${SUBPROJ_SOURCE_FILES})

if (NOT HJ_ENABLE_TNN)
    list(FILTER HEADER_FILES EXCLUDE REGEX "[/\\\\]tnn[/\\\\]")
    list(FILTER HEADER_FILES EXCLUDE REGEX "TNN")
    list(FILTER SOURCE_FILES EXCLUDE REGEX "[/\\\\]tnn[/\\\\]")
    list(FILTER SOURCE_FILES EXCLUDE REGEX "TNN")
endif()

# list(APPEND SOURCE_FILES ${CMAKE_SOURCE_DIR}/Dependencies/generic/spdlog/details/registry-inl.cc)

MAKE_COMMON_CONFIG()

add_library(${PROJ_NAME} STATIC ${SOURCE_FILES} ${HEADER_FILES})

target_link_libraries(${PROJ_NAME} PRIVATE HJConvert HJUtils LIBYUV yyjson LIBSPDLOG LIBSTB LIBREFLCPP)
if (IOS)
    target_link_libraries(${PROJ_NAME} PRIVATE "-framework Foundation" "-framework CoreFoundation" "-framework CoreGraphics" "-framework CoreVideo" "-framework CoreImage" "-framework CoreML" "-framework CoreMedia" "-framework VideoToolbox" "-framework Vision")
endif()

if (HJ_ENABLE_TNN)
    target_compile_definitions(${PROJ_NAME} PRIVATE HJ_ENABLE_TNN)
endif()

if (WINDOWS OR ANDROID OR IOS OR HarmonyOS)
    if (HJ_ENABLE_TNN)
        target_link_libraries(${PROJ_NAME} PRIVATE LIBTNN)
    endif()
    target_link_libraries(${PROJ_NAME} PRIVATE LIBNCNN)
endif()

if (HarmonyOS AND TARGET LIBMINDSPORE)
    target_link_libraries(${PROJ_NAME} PRIVATE LIBMINDSPORE)
    target_compile_definitions(${PROJ_NAME} PRIVATE HJ_HAS_MINDSPORE_LITE=1)
endif()

target_include_directories(${PROJ_NAME} INTERFACE ${${PROJ_NAME}_INCLUDE_DIRS})

HJ_CONFIG_FRAMEWORK(${PROJ_NAME})

set_target_properties(${PROJ_NAME} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${ARCHIVE_output}
    LIBRARY_OUTPUT_DIRECTORY ${library_output}
    RUNTIME_OUTPUT_DIRECTORY ${runtime_output})
	
ProjFolder(${PROJ_NAME} "src")
