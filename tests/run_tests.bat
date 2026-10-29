@echo off
setlocal
set "PATH=D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;%PATH%"
if not exist "%~dp0build\release\test_runner.exe" (
    echo Building test runner...
    if not exist "%~dp0build" mkdir "%~dp0build"
    pushd "%~dp0build"
    qmake ..\tests.pro
    mingw32-make -j4
    popd
)
"%~dp0build\release\test_runner.exe" %*
exit /b %ERRORLEVEL%
