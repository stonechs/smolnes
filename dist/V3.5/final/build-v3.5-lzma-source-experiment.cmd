@echo off
setlocal
for %%I in ("%~dp0..\..\..") do set "ROOT=%%~fI"
set "DIST=%~dp0"
set "SRC=%DIST%source"
set "OBJ=%DIST%build-support\objects"
set "MAP=%DIST%build-support\maps"
set "EXP=%DIST%build-support\lzma-source-experiment"
set "LZMA_SRC=D:\2026work\LzmaCApi\SRC"
set "MALLOC_LIB=%OBJ%\msvcrt-malloc.lib"
set "CLANG=C:\Users\maco\AppData\Local\Programs\LLVM\bin\clang.exe"
set "LLD=C:\Users\maco\AppData\Local\Programs\LLVM\bin\lld-link.exe"
set "NASM=C:\Users\maco\AppData\Local\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin\nasm.exe"
set "CRINKLER=%ROOT%\tools\crinkler\crinkler30b\Win32\Crinkler.exe"
if not exist "%EXP%" mkdir "%EXP%"

call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x86 -host_arch=x64 >nul
if errorlevel 1 exit /b 1
set "FLAGS=--target=i686-pc-windows-msvc -Oz -fms-extensions -fms-compatibility -fms-compatibility-version=19.44 -fno-stack-protector -fno-unwind-tables -fno-asynchronous-unwind-tables -ffunction-sections -fdata-sections -DNDEBUG -I%DIST%vendor\lzmacapi\SRC"
"%CLANG%" %FLAGS% -c "%LZMA_SRC%\sevenzip_single_fast.c" -o "%EXP%\sevenzip_single_fast.obj"
if errorlevel 1 exit /b 1
"%NASM%" -f win32 -O9 "%LZMA_SRC%\lzma_static_x86.asm" -o "%EXP%\lzma_static_x86.obj"
if errorlevel 1 exit /b 1

"%LLD%" /nologo /NODEFAULTLIB /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup /OPT:REF /OPT:ICF /INCREMENTAL:NO /DYNAMICBASE:NO /FIXED /MACHINE:X86 /MERGE:.rdata=.text /MERGE:.data=.text /MERGE:.idata=.text /SECTION:.text,ERW /MAP:"%MAP%\v35-x86-source-lld.map" /OUT:"%EXP%\smolnes-v3.5-source-x86.exe" "%OBJ%\v35-x86-main.obj" "%OBJ%\v35-x86-apu.obj" "%OBJ%\v35-x86-audio.obj" "%EXP%\sevenzip_single_fast.obj" "%EXP%\lzma_static_x86.obj" "%MALLOC_LIB%" kernel32.lib user32.lib gdi32.lib comdlg32.lib winmm.lib libcmt.lib
if errorlevel 1 exit /b 1
"%CRINKLER%" /NODEFAULTLIB /ENTRY:mainCRTStartup /SUBSYSTEM:WINDOWS /COMPMODE:FAST /HASHSIZE:32 /HASHTRIES:100 /TRANSFORM:CALLS /OUT:"%EXP%\smolnes-v3.5-source-x86-crinkler.exe" "%OBJ%\v35-x86-main.obj" "%OBJ%\v35-x86-apu.obj" "%OBJ%\v35-x86-audio.obj" "%EXP%\sevenzip_single_fast.obj" "%EXP%\lzma_static_x86.obj" "%MALLOC_LIB%" kernel32.lib user32.lib gdi32.lib comdlg32.lib winmm.lib libcmt.lib
if errorlevel 1 exit /b 1
endlocal
