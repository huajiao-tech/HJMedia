cmake_minimum_required(VERSION 3.15.0)

project(HJMedia)

# root path
set(HJ_ROOT_PATH ${CMAKE_SOURCE_DIR})
set(HJ_EXTERNALS_PATH ${HJ_ROOT_PATH}/externals)
set(HJ_THIRDPARTY_PATH ${HJ_ROOT_PATH}/third_party)
set(HJ_OUT_PATH ${PROJECT_BINARY_DIR}/out/libs)
set(CMAKE_MODULE_PATH ${HJ_ROOT_PATH}/cmake ${HJ_ROOT_PATH}/cmake/Modules/)
include(UtilsMacros)
include(UtilsHelp)  #before BuildSets
include(BuildSets)
include(warnings)
#message(STATUS "HJ_ROOT_PATH =${HJ_ROOT_PATH} HJ_EXTERNALS_PATH =${HJ_EXTERNALS_PATH} HJ_OUT_PATH =${HJ_OUT_PATH}")

set(library_output ${HJ_OUT_PATH})
set(ARCHIVE_output ${HJ_OUT_PATH})
set(runtime_output ${HJ_OUT_PATH})

if(UNIX AND NOT APPLE)
    # for Linux, BSD, Solaris, Minix
	if (NOT ANDROID AND NOT HarmonyOS)
		set(LINUX TRUE)
	endif()
endif()

message(STATUS "CMAKELIST LINUX:{LINUX}")

if (WIN32)
	add_definitions(-DWIN32_LIB)
	add_definitions(-DWorkDirResource="${CMAKE_SOURCE_DIR}/examples/Windows/resource")

	if(CMAKE_CL_64)
		set(systemStyle "x64")
	else()
		set(systemStyle "x86")
	endif()

	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /Zi")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zi")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
elseif (ANDROID)
	add_definitions(-DANDROID_LIB)
	
	if (${ANDROID_ABI} MATCHES "armeabi-v7a")
		set(systemStyle "armeabi-v7a")
	elseif(${ANDROID_ABI} MATCHES "arm64-v8a")
		set(systemStyle "arm64-v8a")
	endif()
	add_compile_options(-fPIC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fpic")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpic")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=deprecated-declarations -Wno-deprecated-declarations -Wno-conversion-null -Wno-error=register")
elseif (IOS)
	add_definitions(-DIOS_LIB)

	set(CMAKE_XCODE_ATTRIBUTE_GCC_VERSION "com.apple.compilers.llvm.clang.1_0")
	set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++17")
	set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
	set(CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "12.0")
	set(CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "48E3LB8PD7")
	# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden" CACHE STRING "C compiler flags" FORCE)
	# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=deprecated-declarations -Wno-deprecated-declarations -Wno-conversion-null -Wno-error=register -fvisibility=hidden -fvisibility-inlines-hidden")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=deprecated-declarations -Wno-deprecated-declarations -Wno-conversion-null -Wno-error=register")
elseif(MACOSX)
	add_definitions(-DMACOS_LIB)

	set(CMAKE_XCODE_ATTRIBUTE_GCC_VERSION "com.apple.compilers.llvm.clang.1_0")
	set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++17")
	set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
	# set(CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "9.0")
	# set(CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "48E3LB8PD7")

	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fpic")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpic")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=deprecated-declarations -Wno-deprecated-declarations -Wno-conversion-null -Wno-error=register")
elseif (LINUX)
	add_definitions(-DLINUX_LIB)

	# if(CMAKE_CL_64)
		set(systemStyle "x64")
		MESSAGE( STATUS "OS IS x64")
	# else()
	# 	set(systemStyle "x86")
	# 	MESSAGE( STATUS "OS IS X86")
	# endif()

	# set(CMAKE_CXX_STANDARD_REQUIRED ON)
	# set(CMAKE_CXX_STANDARD 14)

	# add_compile_options(-fPIC)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fpic")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=deprecated-declarations -Wno-deprecated-declarations -Wno-conversion-null")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpic")
	# set(CUDA_NVCC_FLAGS "${CUDA_NVCC_FLAGS} -arch=sm_60 -std=c++11 -O3 -rdc=true -Xcompiler -fPIC")
	set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -fpic")
	# set(CMAKE_CXX_FLAGS "-Wno-error=deprecated-declarations -Wno-deprecated-declarations")
	# # set(CMAKE_EXE_LINKER_FLAGS "-lc++abi")
	# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -lc++abi")
	# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++")

elseif(HarmonyOS)
	set(systemStyle "arm64-v8a")
	set(libDepends ${libDepends}/harmony)
	add_definitions(-DHarmony_LIB)   #add_definitions(-DHarmonyOS)  
	
	set(CMAKE_CXX_STANDARD 17)
	set(CMAKE_CXX_STANDARD_REQUIRED ON)  
	set(CMAKE_CXX_EXTENSIONS OFF)        
	message(STATUS "------------OHOS_ARCH ${OHOS_ARCH}")
	#set(OHOS_ARCH "aarch64")  #ohos.toolchain.cmake OHOS_ARCH arm64-v8a, unrecognized aarch64

	set(CMAKE_SYSTEM_PROCESSOR "aarch64")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fpic -march=armv8-a")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpic -march=armv8-a")


endif() 

#***third party libraries*******************************************************************************#
add_subdirectory(third_party/sonic)
add_subdirectory(third_party/soundtouch)
add_subdirectory(third_party/librtmp)
add_subdirectory(third_party/yyjson)
add_subdirectory(third_party/fdk-aac)

#***subdirectories*******************************************************************************#
add_subdirectory(src/utils)
# add_subdirectory(src/platform)
add_subdirectory(src/media)
add_subdirectory(src/plugins)
add_subdirectory(src/graphs)

add_subdirectory(src/entry/pusher)
add_subdirectory(src/entry/player)

# add_subdirectory(src/core)
# if(WINDOWS OR MACOSX)
# 	add_subdirectory(src/gui)
# 	add_subdirectory(examples/windows/XMediaTools)
# endif()



