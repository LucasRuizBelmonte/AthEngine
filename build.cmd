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
conan install . -of=build --build=missing -s build_type=Debug
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=build\conan_toolchain.cmake
cmake --build build --config Debug

build\Debug\athengine.exe

endlocal