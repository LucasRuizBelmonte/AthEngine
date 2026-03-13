@echo off
setlocal
taskkill /IM athengine_editor.exe /F >nul 2>&1
taskkill /IM athengine_runtime.exe /F >nul 2>&1

for /f "usebackq delims=" %%i in (`
  "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -property installationPath
`) do set VS_PATH=%%i

call "%VS_PATH%\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64

if exist build rmdir /s /q build

conan install . -of=build --build=missing -s build_type=Debug -s arch=x86_64 -s compiler=msvc -s compiler.version=193 -s compiler.runtime=dynamic -s compiler.cppstd=20 -o imgui/*:docking=True -c tools.cmake.cmaketoolchain:generator=Ninja -c tools.cmake.cmaketoolchain:toolset_arch=x64
if errorlevel 1 exit /b %errorlevel%
cmake --preset conan-debug
if errorlevel 1 exit /b %errorlevel%
cmake --build --preset conan-debug
if errorlevel 1 exit /b %errorlevel%

endlocal
