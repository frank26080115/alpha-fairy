@echo off
setlocal
for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "CAM_TIME_VS=%%i"
if not defined CAM_TIME_VS (
    echo Visual Studio with the C++ tools is required.
    exit /b 1
)
call "%CAM_TIME_VS%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b %errorlevel%
cd /d "%~dp0"
cl /nologo /std:c++17 /EHsc /W4 wpd_probe.cpp /Fe:wpd_probe.exe /link ole32.lib oleaut32.lib PortableDeviceGuids.lib propsys.lib
