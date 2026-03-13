@echo off
setlocal EnableExtensions

if "%~1"=="" goto :usage

set "FLOW=%~1"
set "BUILD_TYPE="
set "CONAN_OUT="
set "BUILD_DIR="
set "CONFIG_PRESET="
set "BUILD_PRESET="
set "DIST_DIR="

if /I "%FLOW%"=="editor-debug" (
    set "BUILD_TYPE=Debug"
    set "CONAN_OUT=build\conan\editor-debug"
    set "BUILD_DIR=build\editor\debug"
    set "CONFIG_PRESET=editor-debug"
    set "BUILD_PRESET=editor-debug"
) else if /I "%FLOW%"=="runtime-debug" (
    set "BUILD_TYPE=Debug"
    set "CONAN_OUT=build\conan\runtime-debug"
    set "BUILD_DIR=build\runtime\debug"
    set "CONFIG_PRESET=runtime-debug"
    set "BUILD_PRESET=runtime-debug"
) else if /I "%FLOW%"=="runtime-release" (
    set "BUILD_TYPE=Release"
    set "CONAN_OUT=build\conan\runtime-release"
    set "BUILD_DIR=build\runtime\release"
    set "CONFIG_PRESET=runtime-release"
    set "BUILD_PRESET=runtime-release"
    set "DIST_DIR=dist\runtime\release"
) else (
    goto :usage
)

call :load_vs
if errorlevel 1 exit /b %errorlevel%

if exist "%CONAN_OUT%" rmdir /s /q "%CONAN_OUT%"
if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
if defined DIST_DIR if exist "%DIST_DIR%" rmdir /s /q "%DIST_DIR%"

conan install . ^
    -of="%CONAN_OUT%" ^
    --build=missing ^
    -s build_type=%BUILD_TYPE% ^
    -s arch=x86_64 ^
    -s compiler=msvc ^
    -s compiler.version=193 ^
    -s compiler.runtime=dynamic ^
    -s compiler.cppstd=20 ^
    -c tools.cmake.cmaketoolchain:generator=Ninja ^
    -c tools.cmake.cmaketoolchain:toolset_arch=x64
if errorlevel 1 exit /b %errorlevel%

if exist CMakeUserPresets.json del /q CMakeUserPresets.json

cmake --preset "%CONFIG_PRESET%"
if errorlevel 1 exit /b %errorlevel%
cmake --build --preset "%BUILD_PRESET%"
if errorlevel 1 exit /b %errorlevel%

if defined DIST_DIR (
    cmake --install "%BUILD_DIR%" --prefix "%DIST_DIR%" --config "%BUILD_TYPE%"
    if errorlevel 1 exit /b %errorlevel%
)

echo Build flow "%FLOW%" completed.
if defined DIST_DIR echo Runtime distributable output: %DIST_DIR%

endlocal
exit /b 0

:load_vs
set "VS_PATH="
for /f "usebackq delims=" %%i in (`
  "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -property installationPath
`) do set "VS_PATH=%%i"

if not defined VS_PATH (
    echo ERROR: Visual Studio installation was not found.
    exit /b 1
)

call "%VS_PATH%\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul
if errorlevel 1 (
    echo ERROR: Failed to initialize Visual Studio build environment.
    exit /b 1
)

exit /b 0

:usage
echo Usage:
echo   build.cmd editor-debug
echo   build.cmd runtime-debug
echo   build.cmd runtime-release
exit /b 1
