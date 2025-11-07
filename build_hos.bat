@echo off
setlocal enabledelayedexpansion

set PROJECT_DIR=%~dp0
set SRC_DIR=%PROJECT_DIR:~0,-1%
set BUILD_DIR=%PROJECT_DIR%build_hos

echo %PROJECT_DIR%
echo %BUILD_DIR%
echo %SRC_DIR%

set HARMONY_SDK_PATH=%ProgramFiles%\Huawei\DevEco Studio\sdk\default\openharmony\native
set HARMONY_CMAKE_PATH=%HARMONY_SDK_PATH%\build-tools\cmake\bin 
set HARMONY_NINJA_PATH=%HARMONY_CMAKE_PATH% 

set PATH=%HARMONY_CMAKE_PATH%;%HARMONY_NINJA_PATH%;%PATH%

where cmake.exe >nul 2>&1
if %errorlevel% neq 0 (
    echo 错误: 找不到 cmake.exe
    echo 请检查路径: %HARMONY_CMAKE_PATH%
    pause
    exit /b 1
)

REM cd %BUILD_DIR%

"%HARMONY_CMAKE_PATH%\cmake.exe" -G Ninja ^
    -DCMAKE_TOOLCHAIN_FILE="%HARMONY_SDK_PATH%\build\cmake\ohos.toolchain.cmake" ^
    -DOHOS_ARCH="arm64-v8a" ^
    -DOHOS_PLATFORM="OHOS" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -S "%SRC_DIR%" ^
	-B "%BUILD_DIR%"

pushd "%BUILD_DIR%"
cmake --build .

echo %BUILD_DIR%\output\libs
echo %SRC_DIR%\examples\harmony\hjplayer\libs\arm64-v8a

copy /y %BUILD_DIR%\output\libs\libjplayer.so %SRC_DIR%\examples\harmony\hjplayer\libs\arm64-v8a\libjplayer.so
copy /y %BUILD_DIR%\output\libs\libpublisher.so %SRC_DIR%\examples\harmony\hjplayer\libs\arm64-v8a\libpublisher.so

popd