@echo off
setlocal
set "PATH=D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;%PATH%"
if not exist "%~dp0build_stress" mkdir "%~dp0build_stress"
pushd "%~dp0build_stress"
qmake ..\standalone_m1_stress.pro
mingw32-make -j4
if %ERRORLEVEL% neq 0 (
    echo Compilation failed!
    popd
    exit /b %ERRORLEVEL%
)
popd
if exist "%~dp0build_stress\release\standalone_m1_stress.exe" (
    "%~dp0build_stress\release\standalone_m1_stress.exe" %*
) else (
    "%~dp0build_stress\standalone_m1_stress.exe" %*
)
exit /b %ERRORLEVEL%

