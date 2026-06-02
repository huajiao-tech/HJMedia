@echo off
setlocal EnableExtensions EnableDelayedExpansion

pushd "%~dp0"

set "DEST_ROOT=..\obs-studio\obs-studio\deps\HJRender"
set "DEST_LIB=%DEST_ROOT%\lib\x64\Debug"
set "DEST_LIB_RELEASE=%DEST_ROOT%\lib\x64\Release"
set "DEST_INC=%DEST_ROOT%\include"

set "DEST_INF_ROOT=..\obs-studio\obs-studio\deps\HJInference"
set "DEST_INF_LIB=%DEST_INF_ROOT%\lib\x64\Debug"
set "DEST_INF_LIB_RELEASE=%DEST_INF_ROOT%\lib\x64\Release"
set "DEST_INF_INC=%DEST_INF_ROOT%\include"
set "DEST_PDB=..\obs-studio\obs-studio\build-windows-x64\rundir\Release_PDB\bin\64bit"


if not exist "%DEST_LIB%" mkdir "%DEST_LIB%"
if not exist "%DEST_LIB_RELEASE%" mkdir "%DEST_LIB_RELEASE%"
if not exist "%DEST_INC%" mkdir "%DEST_INC%"

if not exist "%DEST_INF_LIB%" mkdir "%DEST_INF_LIB%"
if not exist "%DEST_INF_LIB_RELEASE%" mkdir "%DEST_INF_LIB_RELEASE%"
if not exist "%DEST_INF_INC%" mkdir "%DEST_INF_INC%"

call :copy_file "build-windows-x64\out\libs\x64\Debug\HJRender.dll" "%DEST_LIB%" || exit /b 1
call :copy_file "build-windows-x64\out\libs\x64\Debug\HJRender.lib" "%DEST_LIB%" || exit /b 1
call :copy_file "build-windows-x64\out\libs\x64\Release\HJRender.dll" "%DEST_LIB_RELEASE%" || exit /b 1
call :copy_file "build-windows-x64\out\libs\x64\Release\HJRender.lib" "%DEST_LIB_RELEASE%" || exit /b 1

call :copy_file "build-windows-x64\out\libs\x64\Debug\HJInference.dll" "%DEST_INF_LIB%" || exit /b 1
call :copy_file "build-windows-x64\out\libs\x64\Debug\HJInference.lib" "%DEST_INF_LIB%" || exit /b 1
call :copy_file "build-windows-x64\out\libs\x64\Release\HJInference.dll" "%DEST_INF_LIB_RELEASE%" || exit /b 1
call :copy_file "build-windows-x64\out\libs\x64\Release\HJInference.lib" "%DEST_INF_LIB_RELEASE%" || exit /b 1

call :copy_file "build-windows-x64\out\libs\x64\Release\HJRender.pdb" "%DEST_PDB%" || exit /b 1
call :copy_file "build-windows-x64\out\libs\x64\Release\HJInference.pdb" "%DEST_PDB%" || exit /b 1

call :copy_file "src\entry\HJCommonInterface.h" "%DEST_INC%" || exit /b 1
call :copy_file "src\entry\render\HJRenderGraphExport.h" "%DEST_INC%" || exit /b 1
call :copy_file "src\entry\render\HJRenderContextExport.h" "%DEST_INC%" || exit /b 1
call :copy_file "src\utils\base\HJExport.h" "%DEST_INC%" || exit /b 1

call :copy_file "src\entry\HJCommonInterface.h" "%DEST_INF_INC%" || exit /b 1
call :copy_file "src\entry\inference\HJFaceDetectExport.h" "%DEST_INF_INC%" || exit /b 1
call :copy_file "src\entry\inference\HJInferenceContextExport.h" "%DEST_INF_INC%" || exit /b 1
call :copy_file "src\utils\base\HJExport.h" "%DEST_INF_INC%" || exit /b 1

popd

pause
endlocal

goto :eof

:copy_file
set "SRC=%~1"
set "DST=%~2"
if "!DST:~-1!"=="\" set "DST=!DST:~0,-1!"
if not exist "!SRC!" (
  echo [FAIL] !SRC! ^(missing source^)
  exit /b 1
)
if not exist "!DST!" (
  echo [FAIL] !DST! ^(missing destination^)
  exit /b 1
)
copy /y "!SRC!" "!DST!" >nul 2>nul
set "RC=!errorlevel!"
if !RC! NEQ 0 (
  echo [FAIL] !SRC! ^(errorlevel=!RC!^)
  exit /b !RC!
)
echo [OK] !SRC!
exit /b 0
