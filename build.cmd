@echo off
setlocal
taskkill /IM athengine.exe /F >nul 2>&1
:: Locate Visual Studio
for /f "usebackq delims=" %%i in (`
  "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -property installationPath
`) do set VS_PATH=%%i

:: Activate MSVC x64
call "%VS_PATH%\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64

:: Clean previous build
if exist build rmdir /s /q build

:: Conan install (generates presets)

conan install . -of=build --build=missing -s build_type=Debug -c tools.cmake.cmaketoolchain:generator=Ninja
cmake --preset conan-debug
cmake --build --preset conan-debug

build/athengine.exe

endlocal