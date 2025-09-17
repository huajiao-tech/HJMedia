set(PROJ_NAME XMediaTools)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

include_directories(${HJ_EXTERNALS_PATH}/generic)
include_directories(${HJ_EXTERNALS_PATH}/generic/ffmpeg)
include_directories(${HJ_DEPS_DIR}/ffmpeg/include)
link_directories(${HJ_DEPS_DIR}/ffmpeg/lib/${ARCHS_NAME})

file(GLOB CUR_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB CUR_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cc")

list(APPEND HEADER_FILES ${CUR_HEADER_FILES})
list(APPEND SOURCE_FILES ${CUR_SOURCE_FILES})
if(WINDOWS)
    list(APPEND SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/windows/${PROJ_NAME}.rc")
endif()

# MESSAGE( STATUS "${PROJ_NAME} SOURCE_FILES : ${SOURCE_FILES}, HEADER_FILES : ${HEADER_FILES}")

MAKE_COMMON_CONFIG(${SOURCE_FILES})

# set(CMAKE_EXE_LINKER_FLAGS"${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
add_executable(${PROJ_NAME} ${SOURCE_FILES} ${HEADER_FILES})

if(WINDOWS)
    target_link_libraries(${PROJ_NAME} 
        Crypt32 IPHLPAPI opengl32 dwmapi
        HJGraphs HJPlugins
        HJGui ImGuiFileDialog ImFileDialog implot imgui HJCore HJLinks HJMedia HJStat HJUtils LIBGLFW LIBSPDLOG
    )
    target_link_libraries(${PROJ_NAME} debug libavdeviced optimized libavdevice)
    target_link_libraries(${PROJ_NAME} debug libavfilterd optimized libavfilter)
    target_link_libraries(${PROJ_NAME} debug libavformatd optimized libavformat)
    target_link_libraries(${PROJ_NAME} debug libavcodecd optimized libavcodec)
    # target_link_libraries(${PROJ_NAME} debug libpostprocd optimized libpostproc)
    target_link_libraries(${PROJ_NAME} debug libswresampled optimized libswresample)
    target_link_libraries(${PROJ_NAME} debug libswscaled optimized libswscale)
    target_link_libraries(${PROJ_NAME} debug libavutild optimized libavutil)
    #
    # target_link_libraries(${PROJ_NAME} debug exception_handlerd optimized exception_handler)
else()
    target_link_libraries(${PROJ_NAME} 
        LIBABSL 
        LIBPROTOBUF
        HJCore
        HJStat
        HJUtils
        HJGui
        ${DEPS_FFMPEG_LIB_DIR}/libavdevice.a
        ${DEPS_FFMPEG_LIB_DIR}/libavfilter.a
        ${DEPS_FFMPEG_LIB_DIR}/libavformat.a
        ${DEPS_FFMPEG_LIB_DIR}/libavcodec.a
        ${DEPS_FFMPEG_LIB_DIR}/libswresample.a
        ${DEPS_FFMPEG_LIB_DIR}/libswscale.a
        ${DEPS_FFMPEG_LIB_DIR}/libavutil.a
        x264
        x265
        ssl
        crypto
        rist
        srt
        LIBGLFW
        iconv
        bz2
        z
        MQMonitor
        SLPlat 
        SLBase
    )
endif()

if(WINDOWS)
    set_target_properties(${PROJ_NAME} PROPERTIES LINK_FLAGS "/SUBSYSTEM:CONSOLE /SAFESEH:NO")
elseif(APPLE)
    set_target_properties(${PROJ_NAME} PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks -framework Foundation -framework Accelerate -framework AVFoundation -framework VideoToolbox -framework AudioToolbox -framework CoreGraphics -framework CoreVideo -framework CoreImage -framework Metal -framework AppKit -framework CoreAudio -framework CoreMedia -framework OpenGL -framework IOKit -framework Cocoa")
endif(WINDOWS)

set_target_properties(${PROJ_NAME} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${ARCHIVE_output}
    LIBRARY_OUTPUT_DIRECTORY ${library_output}
    RUNTIME_OUTPUT_DIRECTORY ${runtime_output})

ProjFolder(${PROJ_NAME} "examples")
