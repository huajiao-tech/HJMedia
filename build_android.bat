@echo off
cls

SET SOURCE_DIR=.
SET OUTPUT_DIR=build_aos
SET ANDROID_PATH=%USERPROFILE%\AppData\Local\Android\Sdk
SET NDK_PATH=%ANDROID_PATH%\ndk\25.1.8937393
SET CMAKE_TOOLCHAIN=%NDK_PATH%\build\cmake\android.toolchain.cmake
SET CMAKE_PATH=%ANDROID_PATH%\cmake\3.22.1
@REM SET CPU_ABI=arm64-v8a
@REM SET CPU_ABI=armeabi-v7a

REM rm -rf %OUTPUT_DIR%
mkdir %OUTPUT_DIR%
@REM cd %OUTPUT_DIR%

if not defined ORIGPATH set ORIGPATH=%PATH%
SET PATH=%CMAKE_PATH%\bin;%ANDROID_PATH%\tools;%ANDROID_PATH%\platform-tools;%ORIGPATH%

call :Build_Target arm64-v8a
call :Build_Target armeabi-v7a
goto :eof

:Build_Target
echo -------------------- start build %1  --------------------
SET MK_DIR=%OUTPUT_DIR%_%1
mkdir %MK_DIR%
cd %MK_DIR%
cmake ^
      -GNinja ^
      -DCMAKE_TOOLCHAIN_FILE=%CMAKE_TOOLCHAIN% ^
      -DANDROID_NDK=%NDK_PATH% ^
      -DCMAKE_MAKE_PROGRAM=%CMAKE_PATH%\bin\ninja.exe ^
      -DCMAKE_BUILD_TYPE=Release ^
      -DCMAKE_SHARED_LINKER_FLAGS=-Wl,-Bsymbolic ^
      -DANDROID_ABI=%1 ^
      -DANDROID_ARM_NEON=ON ^
      -DANDROID_NATIVE_API_LEVEL=21 ^
      -DANDROID_APP_PIE=TRUE ^
      -DANDROID_STL_FORCE_FEATURES=TRUE ^
      -DANDROID_FORCE_ARM_BUILD=TRUE ^
	-DANDROID_UNIFIED_HEADERS=TRUE ^
	-DCMAKE_HOST_SYSTEM_NAME=Windows ^
      -DCMAKE_INSTALL_PREFIX=install ^
      -DANDROID_STL=c++_static ^
      -DCMAKE_CXX_FLAGS=-Wno-c++11-narrowing ^
      -DANDROID_TOOLCHAIN=clang ^
      ../%SOURCE_DIR%

cmake --build .
REM cmake --build . --target install

cd ..

xcopy %MK_DIR%\out %OUTPUT_DIR% /S /E /I /Y /Q
REM rd %MK_DIR% /S /Q
@REM rm -rf %MK_DIR%

echo -------------------- end build %1  --------------------


REM cmake -P cmake_install.cmake

@REM cd ..


SET CURRENT_DIR=%~dp0
echo -------------------- hello============ --------------------
echo %CURRENT_DIR%
echo %OUTPUT_DIR%

copy /y %CURRENT_DIR%\%OUTPUT_DIR%\libs\arm64-v8a\libHJRender.so     %CURRENT_DIR%\demo\faceu\android\Export\app\src\main\jniLibs\arm64-v8a\libHJRender.so
copy /y %CURRENT_DIR%\%OUTPUT_DIR%\libs\armeabi-v7a\libHJRender.so   %CURRENT_DIR%\demo\faceu\android\Export\app\src\main\jniLibs\armeabi-v7a\libHJRender.so
@REM copy /y %CURRENT_DIR%\%OUTPUT_DIR%\libs\arm64-v8a\libjplayer.so     %CURRENT_DIR%\examples\Android\SLMedia\MediaLibs\src\main\jniLibs\arm64-v8a\libjplayer.so
@REM copy /y %CURRENT_DIR%\%OUTPUT_DIR%\libs\arm64-v8a\libpublisher.so   %CURRENT_DIR%\examples\Android\SLMedia\MediaLibs\src\main\jniLibs\arm64-v8a\libpublisher.so

@REM copy /y %CURRENT_DIR%\%OUTPUT_DIR%\libs\armeabi-v7a\libjplayer.so   %CURRENT_DIR%\examples\Android\SLMedia\MediaLibs\src\main\jniLibs\armeabi-v7a\libjplayer.so
@REM copy /y %CURRENT_DIR%\%OUTPUT_DIR%\libs\armeabi-v7a\libpublisher.so %CURRENT_DIR%\examples\Android\SLMedia\MediaLibs\src\main\jniLibs\armeabi-v7a\libpublisher.so


@REM copy /y %CURRENT_DIR%\Dependencies\android\ffmpeg4.1.3\lib\arm64-v8a\libqhffmpeg.so %CURRENT_DIR%\examples\Android\SLMedia\MediaLibs\src\main\jniLibs\arm64-v8a\libqhffmpeg.so
@REM copy /y %CURRENT_DIR%\Dependencies\android\ffmpeg4.1.3\lib\armeabi-v7a\libqhffmpeg.so %CURRENT_DIR%\examples\Android\SLMedia\MediaLibs\src\main\jniLibs\armeabi-v7a\libqhffmpeg.so



@REM copy /y %CURRENT_DIR%\%OUTPUT_DIR%\libs\arm64-v8a\libMQMonitor.so %CURRENT_DIR%\..\huajiao_android_main\living_android\src\main\jniLibs\arm64-v8a\libMQMonitor.so    
@REM copy /y %CURRENT_DIR%\%OUTPUT_DIR%\libs\armeabi-v7a\libMQMonitor.so %CURRENT_DIR%\..\huajiao_android_main\living_android\src\main\jniLibs\armeabi-v7a\libMQMonitor.so

@REM copy /y %CURRENT_DIR%\%OUTPUT_DIR%\libs\arm64-v8a\libSLEntry.so %CURRENT_DIR%\..\huajiao_android_main\living_android\src\main\jniLibs\arm64-v8a\libSLEntry.so    
@REM copy /y %CURRENT_DIR%\%OUTPUT_DIR%\libs\armeabi-v7a\libSLEntry.so %CURRENT_DIR%\..\huajiao_android_main\living_android\src\main\jniLibs\armeabi-v7a\libSLEntry.so        

goto:eof



REM armeabi-v7a
REM copy /y G:\code\ogre\ogre_20180803\ogre\ogre_out\OgreEngine\libs\armeabi-v7a\libRenderEngine.so G:\code\mediapipe\facemesh\app\src\main\jniLibs\armeabi-v7a\libRenderEngine.so

REM arm64-v8a
REM copy /y E:\code\generalGit\slmedia\jni\build_android64\output\libs\libSLEntry.so E:\code\generalGit\slmedia\app\src\main\jniLibs\arm64-v8a\libSLEntry.so
REM copy /y G:\code\ogre\ogre_20180803\ogre\ogre_out\OgreEngine\libs\arm64-v8a\libRenderEngine.so E:\code\five\libs\arm64-v8a\libRenderEngine.so
REM copy /y G:\code\ogre\ogre_20180803\ogre\ogre_out\OgreEngine\libs\arm64-v8a\libRenderEngine.so G:\code\mediapipe\handtrack\app\src\main\jniLibs\arm64-v8a\libRenderEngine.so