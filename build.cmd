@echo off
setlocal
taskkill /IM athengine.exe /F >nul 2>&1

for /f "usebackq delims=" %%i in (`
  "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -property installationPath
`) do set VS_PATH=%%i

call "%VS_PATH%\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64

if exist build rmdir /s /q build

conan install . -of=build --build=missing -s build_type=Debug -s arch=x86_64 -o imgui/*:docking=True -c tools.cmake.cmaketoolchain:generator=Ninja -c tools.cmake.cmaketoolchain:toolset_arch=x64
cmake --preset conan-debug
cmake --build --preset conan-debug

build/athengine.exe

endlocal