@ECHO OFF
@SETLOCAL ENABLEDELAYEDEXPANSION

REM ===== user-configurable paths =====
SET SOURCE_DIR=.
SET OUTPUT_DIR=build_aos
SET ANDROID_PATH=%USERPROFILE%\AppData\Local\Android\Sdk
SET NDK_PATH=%ANDROID_PATH%\ndk\25.1.8937393
SET CMAKE_TOOLCHAIN=%NDK_PATH%\build\cmake\android.toolchain.cmake
SET CMAKE_PATH=%ANDROID_PATH%\cmake\3.22.1

REM ===== env setup =====
if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"
if not defined ORIGPATH set ORIGPATH=%PATH%
SET PATH=%CMAKE_PATH%\bin;%ANDROID_PATH%\tools;%ANDROID_PATH%\platform-tools;%ORIGPATH%

if not exist "%CMAKE_TOOLCHAIN%" (
    echo [ERROR] CMAKE_TOOLCHAIN not found: %CMAKE_TOOLCHAIN%
    exit /b 1
)
if not exist "%CMAKE_PATH%\bin\ninja.exe" (
    echo [ERROR] ninja not found: %CMAKE_PATH%\bin\ninja.exe
    exit /b 1
)

call :Build_Target arm64-v8a
if errorlevel 1 exit /b 1
call :Build_Target armeabi-v7a
if errorlevel 1 exit /b 1

echo.
echo Build finished.
echo Static libs:
echo   %CD%\%OUTPUT_DIR%_arm64-v8a\install\lib\libncnn.a
echo   %CD%\%OUTPUT_DIR%_armeabi-v7a\install\lib\libncnn.a
goto :eof

:Build_Target
echo -------------------- start build %1 --------------------
SET MK_DIR=%OUTPUT_DIR%_%1
if not exist "%MK_DIR%" mkdir "%MK_DIR%"
pushd "%MK_DIR%"

SET ABI_NEON_FLAG=
if "%1"=="armeabi-v7a" SET ABI_NEON_FLAG=-DANDROID_ARM_NEON=ON

cmake ^
  -G Ninja ^
  -DCMAKE_TOOLCHAIN_FILE="%CMAKE_TOOLCHAIN%" ^
  -DANDROID_NDK="%NDK_PATH%" ^
  -DCMAKE_MAKE_PROGRAM="%CMAKE_PATH%\bin\ninja.exe" ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_SHARED_LINKER_FLAGS=-Wl,-Bsymbolic ^
  -DANDROID_ABI=%1 ^
  %ABI_NEON_FLAG% ^
  -DANDROID_NATIVE_API_LEVEL=21 ^
  -DANDROID_TOOLCHAIN=clang ^
  -DCMAKE_INSTALL_PREFIX=install ^
  -DANDROID_APP_PIE=TRUE ^
  -DANDROID_STL=c++_static ^
  -DCMAKE_CXX_FLAGS=-Wno-c++11-narrowing ^
  -DNCNN_VULKAN=ON ^
  -DNCNN_OPENMP=ON ^
  -DNCNN_SYSTEM_GLSLANG=OFF ^
  -DNCNN_SHARED_LIB=OFF ^
  -DNCNN_BUILD_TOOLS=OFF ^
  -DNCNN_BUILD_EXAMPLES=OFF ^
  -DNCNN_BUILD_TESTS=OFF ^
  -DNCNN_BUILD_BENCHMARK=OFF ^
  ..\%SOURCE_DIR%
if errorlevel 1 (
  popd
  exit /b 1
)

cmake --build . --config Release --parallel %NUMBER_OF_PROCESSORS%
if errorlevel 1 (
  popd
  exit /b 1
)

cmake --build . --target install --config Release
if errorlevel 1 (
  popd
  exit /b 1
)

popd
exit /b 0


REM   -DNCNN_OPENMP=OFF ^