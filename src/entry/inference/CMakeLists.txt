set(PROJ_NAME HJInference)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

#if (HarmonyOS)
#    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/hsys/bridge)
#endif()


set(PUBLIC_HEADERS
 ${CMAKE_CURRENT_SOURCE_DIR}/../HJCommonInterface.h
 ${CMAKE_CURRENT_SOURCE_DIR}/HJFaceDetectExport.h
 ${CMAKE_CURRENT_SOURCE_DIR}/HJVideoSRExport.h
 ${CMAKE_CURRENT_SOURCE_DIR}/utils/HJFaceSRWrapper.h
 ${CMAKE_CURRENT_SOURCE_DIR}/utils/HJFaceAcquireWrapper.h
 ${CMAKE_CURRENT_SOURCE_DIR}/HJInferenceContextExport.h
 ${CMAKE_SOURCE_DIR}/src/utils/base/HJExport.h)
 
#message (STATUS "PUBLIC_HEADERS:${PUBLIC_HEADERS}")

set(${PROJ_NAME}_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR})
list(APPEND ${PROJ_NAME}_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/../)
list(APPEND ${PROJ_NAME}_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/utils)

add_definitions(-DHJ_INFERENCE_CORE)

if (ANDROID)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/asys)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../utils/asys)
endif()

# message (STATUS "retrieve_files entry")
HJ_SUBDIR_LIST(subdir_names ${CMAKE_CURRENT_SOURCE_DIR})
# message (STATUS "core sub dir names:${subdir_names}")

HJ_RETRIEVE_XFILES(SUBPROJ_SOURCE_FILES ${subdir_names})
HJ_RETRIEVE_HFILES(SUBPROJ_HEADER_FILES ${subdir_names})

file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cc" "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/../*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/../*.cc" "${CMAKE_CURRENT_SOURCE_DIR}/../utils/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/../utils/*.cc")

list(APPEND HEADER_FILES ${SUBPROJ_HEADER_FILES})
list(APPEND HEADER_FILES ${PUBLIC_HEADERS})
list(REMOVE_DUPLICATES HEADER_FILES)
list(APPEND SOURCE_FILES ${SUBPROJ_SOURCE_FILES})

if (WINDOWS)
    list(APPEND HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/../render/HJRenderCoreSingleton.h")
    list(APPEND SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/../render/HJRenderCoreSingleton.cpp")
endif()

#if (ANDROID)
#    file(GLOB ASYS_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/../utils/asys/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/../utils/asys/*.cc")
#	list(APPEND SOURCE_FILES ${ASYS_SOURCE_FILES})
#endif()


list(REMOVE_ITEM SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/../HJEntryBaseRender.cpp")
#list(REMOVE_ITEM SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/../HJEntryContext.cpp")

# list(APPEND SOURCE_FILES ${CMAKE_SOURCE_DIR}/Dependencies/generic/spdlog/details/registry-inl.cc)

MAKE_COMMON_CONFIG()

add_library(${PROJ_NAME} SHARED ${SOURCE_FILES} ${HEADER_FILES})
#add_library(${PROJ_NAME} STATIC ${SOURCE_FILES} ${HEADER_FILES})
if (HarmonyOS)
    set(Harmony_Native_LIBRARY
    libace_napi.z.so libdeviceinfo_ndk.z.so libEGL.so libGLESv3.so libace_ndk.z.so libuv.so libhilog_ndk.z.so
    libnative_media_codecbase.so libnative_media_core.so libnative_media_vdec.so libnative_window.so
    libnative_media_venc.so libnative_media_acodec.so libnative_media_avdemuxer.so libnative_media_avsource.so libnative_media_avmuxer.so
    libohaudio.so libnative_image.so libnative_buffer.so libimage_source.so libpixelmap.so libimage_receiver.so libohimage.so)
    target_link_libraries(${PROJ_NAME} PRIVATE ${Harmony_Native_LIBRARY})

    #target_link_libraries(${PROJ_NAME} PRIVATE LIBX264)
    target_link_libraries(${PROJ_NAME} PRIVATE zlib)   # ffmpeg png encoder need zlib
    target_link_options(${PROJ_NAME} PRIVATE "-Wl,-Bsymbolic")  #ld.lld: error: relocation R_AARCH64_ADR_PREL_PG_HI21 cannot be used against symbol 'ff_tx_tab_2048_float'; recompile with -fPIC
endif()

target_link_libraries(${PROJ_NAME} PRIVATE HJComp HJConvert yyjson HJStat LIBSTB LIBSPDLOG LIBMINICORO HJUtils LIBYUV LIBREFLCPP)

if (WINDOWS OR ANDROID OR IOS OR HarmonyOS)
    target_link_libraries(${PROJ_NAME} PRIVATE HJDetect LIBNCNN)

    if (ANDROID)
        target_compile_options(${PROJ_NAME} PRIVATE -fopenmp)
        target_link_options(${PROJ_NAME} PRIVATE -fopenmp -static-openmp)
    endif()

endif()

if (WINDOWS)
    if (HJ_ENABLE_TNN)
        target_link_libraries(${PROJ_NAME} PRIVATE LIBTNN)
    endif()
    set_target_properties(${PROJ_NAME} PROPERTIES LINK_FLAGS "/SUBSYSTEM:CONSOLE /SAFESEH:NO /DEBUG")  # windows pdb file produce
endif()

if (WINDOWS)
    target_link_libraries(${PROJ_NAME} PRIVATE winmm.lib Crypt32.lib)
endif()  

if (WINDOWS OR MACOSX)
    target_link_libraries(${PROJ_NAME} PRIVATE glfw)
    #target_link_libraries(${PROJ_NAME} PRIVATE implot imgui glfw)
    add_definitions(-DHJ_EXPORT_SET)
endif()

if (ANDROID)
    target_link_libraries(${PROJ_NAME} PRIVATE android jnigraphics c m dl z log EGL GLESv3 OpenSLES)
    if (NOT ANDROID_STUDIO_DEBUG)
		set_target_properties(${PROJ_NAME} PROPERTIES LINK_FLAGS -s)
    endif()
elseif (APPLE)
    set(_hjapple_frameworks "-framework Foundation" "-framework CoreFoundation" "-framework AVFoundation" "-framework CoreGraphics")
    if (IOS OR MACOSX)
        list(APPEND _hjapple_frameworks "-framework IOKit")
    endif()
    if (IOS)
        list(APPEND _hjapple_frameworks "-framework OpenGLES" "-framework CoreVideo" "-framework CoreImage" "-framework CoreML" "-framework Vision")
    else()
        list(APPEND _hjapple_frameworks "-framework OpenGL")
    endif()
    target_link_libraries(${PROJ_NAME} PUBLIC ${_hjapple_frameworks})   
endif()

target_include_directories(${PROJ_NAME} INTERFACE ${${PROJ_NAME}_INCLUDE_DIRS})

#HJ_CONFIG_FRAMEWORK(${PROJ_NAME})
HJ_CONFIG_FRAMEWORK_EX(${PROJ_NAME})

set_target_properties(${PROJ_NAME} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${ARCHIVE_output}
    LIBRARY_OUTPUT_DIRECTORY ${library_output}
    RUNTIME_OUTPUT_DIRECTORY ${runtime_output})

ProjFolder(${PROJ_NAME} "src")
