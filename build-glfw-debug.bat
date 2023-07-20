call cmake -S "%~dp0glfw" -B "%~dp0glfw\build" -A Win32 -DGLFW_BUILD_DOCS=FALSE -DGLFW_BUILD_EXAMPLES=FALSE -DGLFW_BUILD_TESTS=FALSE -DGLFW_INSTALL=FALSE -DUSE_MSVC_RUNTIME_LIBRARY_DLL=FALSE

cd /d "%~dp0"

for /f "usebackq tokens=*" %%i in (`vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
  set InstallDir=%%i
)

if exist "%InstallDir%\Common7\Tools\vsdevcmd.bat" (

    "%InstallDir%\Common7\Tools\vsdevcmd.bat" -arch=x86

    MSBuild.exe "%~dp0\glfw\build\ALL_BUILD.vcxproj" /p:Configuration=Debug /p:Platform="Win32"

    pause
    
)