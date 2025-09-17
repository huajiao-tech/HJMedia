set(LibName HJMainUI)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

file(GLOB CUR_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB CUR_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

list(APPEND HEADER_FILES ${CUR_HEADER_FILES})
list(APPEND SOURCE_FILES ${CUR_SOURCE_FILES})


file(GLOB DIR_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
file(GLOB DIR_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")

list(APPEND HEADER_FILES ${DIR_HEADER_FILES})
list(APPEND SOURCE_FILES ${DIR_SOURCE_FILES})

MESSAGE( STATUS "SOURCE_FILES = ${SOURCE_FILES}")

file(GLOB RC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.rc")

if(MSVC)
	add_definitions( /wd4251 /wd4275 /wd4819 /wd4099 /wd4244 /wd4996 /wd4101 /wd5033)
endif()

#MD shared MFC library and define _AFXDLL
#set(CMAKE_MFC_FLAG 2) 
#add_definitions(-D_AFXDLL)  #MD shared MFC;  MT static MFC, not define _AFXDLL

#MT static MFC library
#set(CMAKE_MFC_FLAG 1)  # 1 for the static MFC library, and 2 for the shared MFC library

add_executable(${LibName} ${SOURCE_FILES} ${HEADER_FILES} ${RC_FILES})
target_link_libraries(${LibName} HJStat HJUtils LIBSPDLOG)

set_target_properties(${LibName} PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS /SAFESEH:NO")

set_target_properties(${LibName} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${ARCHIVE_output}
    LIBRARY_OUTPUT_DIRECTORY ${library_output}
    RUNTIME_OUTPUT_DIRECTORY ${runtime_output})

SettingMFC(${LibName})

ProjFolder(${LibName} "MainUI")

#target_link_libraries(${LibName} SLEntry MTMCore MTMUtils SLPlugin SLPlat SLBase soundtouch)	
#target_link_libraries(${LibName} ${FF_YUV_LIBS_FILES} cuda)
#target_link_libraries(${LibName} ${libssl}/libssl_static.lib ${libssl}/libcrypto_static.lib)
#target_link_libraries(SLJPlayer winmm.lib Crypt32.lib) ##waveout	
	