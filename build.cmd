@echo off
rem Builds hiddriver.xex with the Xbox 360 SDK's command line tools, no Visual Studio needed.
rem Mirrors the "Release Retail|Xbox 360" settings in hiddriver\hiddriver.vcxproj.
rem The main XDK compiler takes its C/C++ runtime headers from Visual Studio 2010, so without VS2010 this uses the
rem XDK's Jul12 TechPreview compiler, which ships its own runtime headers and libraries.
setlocal
if "%XDK%"=="" set "XDK=D:\XDK\full\XDK"
set "TP=%XDK%\TechPreview\Jul12Compiler"
set "PATH=%TP%\bin\win32;%XDK%\bin\win32;%PATH%"
set "INCLUDE=%TP%\include\xbox;%XDK%\include\xbox"
set "LIB=%TP%\lib\xbox;%XDK%\lib\xbox"

rem "build.cmd bootlog" makes a debug build that logs every init step to hiddriver_boot.log on the usb drive
set "DEFINES="
if /i "%~1"=="bootlog" set "DEFINES=/D BOOTLOG"

set "SRC=%~dp0hiddriver"
set "OUT=%~dp0build"
if not exist "%OUT%" mkdir "%OUT%"

rem The PE has to be linked at the load address in xex.xml. Plugins in 0x9xxxxxxx use 4 KB pages, which needs
rem /ALIGN:128,4096, anything lower keeps the default 64 KB pages.
set "BASE="
for /f tokens^=2^ delims^=^" %%a in ('findstr /c:"baseaddr addr" "%SRC%\xex.xml"') do set "BASE=%%a"
if "%BASE%"=="" (echo No baseaddr in xex.xml & goto :fail)
set "ALIGN="
if /i "%BASE:~0,3%"=="0x9" set "ALIGN=/ALIGN:128,4096"
echo Load address %BASE% %ALIGN%

cl /nologo /c /Ox /Ob2 /Oi /Ot /GL /GF /Gy /GS- /MT /W0 /Zi ^
   /D NDEBUG /D _XBOX /D LTCG /D _MBCS %DEFINES% ^
   /I "%SRC%\xkelib" /I "%SRC%" ^
   /Fo"%OUT%\\" /Fd"%OUT%\hiddriver_cl.pdb" ^
   "%SRC%\hid_parser.cpp" "%SRC%\main.cpp" "%SRC%\Detours.cpp" "%SRC%\mapping.cpp" "%SRC%\gip.cpp"
if errorlevel 1 goto :fail

rem /XEX:NO stops link from running imagexex itself without xex.xml, the sysdll it builds below needs the config
link /nologo /DLL /XEX:NO %ALIGN% /BASE:%BASE% /LTCG /OPT:REF /OPT:ICF /DEBUG /RELEASE ^
   /LIBPATH:"%SRC%\xkelib" xkelib.lib xapilib.lib xboxkrnl.lib ^
   /OUT:"%OUT%\hiddriver.dll" /PDB:"%OUT%\hiddriver.pdb" ^
   "%OUT%\hid_parser.obj" "%OUT%\main.obj" "%OUT%\Detours.obj" "%OUT%\mapping.obj" "%OUT%\gip.obj"
if errorlevel 1 goto :fail

imagexex /nologo /CONFIG:"%SRC%\xex.xml" /IN:"%OUT%\hiddriver.dll" /OUT:"%OUT%\hiddriver.xex"
if errorlevel 1 goto :fail

rem imagexex encrypts with the devkit key, retail consoles need the retail key (what `xextool -m r` does)
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\xex-retail.ps1" "%OUT%\hiddriver.xex"
if errorlevel 1 goto :fail

echo.
echo Built %OUT%\hiddriver.xex
exit /b 0

:fail
echo.
echo BUILD FAILED
exit /b 1
