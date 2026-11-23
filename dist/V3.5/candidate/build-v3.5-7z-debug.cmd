@echo off
setlocal
for %%I in ("%~dp0..\..\..") do set "ROOT=%%~fI"
set "DIST=%~dp0"
set "SRC=%ROOT%\src"
set "OBJ=%DIST%build-support\objects"
set "MAP=%DIST%build-support\maps"
set "TEST=%DIST%build-support\test"
set "LZMA_SRC=%ROOT%\src\lzmacapi\SRC"
set "MALLOC_DEF=%DIST%build-support\msvcrt-malloc.def"
set "MALLOC_LIB=%OBJ%\msvcrt-malloc.lib"
set "CLANG=C:\Users\maco\AppData\Local\Programs\LLVM\bin\clang.exe"
set "LLD=C:\Users\maco\AppData\Local\Programs\LLVM\bin\lld-link.exe"
set "LIBTOOL=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x86\lib.exe"
set "NASM=C:\Users\maco\AppData\Local\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin\nasm.exe"
if not exist "%TEST%" mkdir "%TEST%"

call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x86 -host_arch=x64 >nul
if errorlevel 1 exit /b 1
"%LIBTOOL%" /nologo /def:"%MALLOC_DEF%" /machine:x86 /out:"%MALLOC_LIB%"
if errorlevel 1 exit /b 1
set "FLAGS=--target=i686-pc-windows-msvc -Oz -fms-extensions -fms-compatibility -fms-compatibility-version=19.44 -fno-stack-protector -fno-unwind-tables -fno-asynchronous-unwind-tables -ffunction-sections -fdata-sections -DSMOLNES_ENABLE_7Z_LZMA1 -DSMOLNES_7Z_VALIDATE_ONLY"
"%CLANG%" %FLAGS% -DSMOLNES_7Z_DEBUG -c "%SRC%\deobfuscated_win32.c" -o "%OBJ%\v35-x86-debug-main.obj"
if errorlevel 1 exit /b 1
"%CLANG%" %FLAGS% -c "%SRC%\deobfuscated_win32.c" -o "%OBJ%\v35-x86-release-test-main.obj"
if errorlevel 1 exit /b 1
"%CLANG%" %FLAGS% -c "%SRC%\apu.c" -o "%OBJ%\v35-x86-debug-apu.obj"
if errorlevel 1 exit /b 1
"%CLANG%" %FLAGS% -c "%SRC%\audio_win32.c" -o "%OBJ%\v35-x86-debug-audio.obj"
if errorlevel 1 exit /b 1
"%CLANG%" %FLAGS% -DLZMA_C_API_DEBUG -I"%LZMA_SRC%" -c "%LZMA_SRC%\sevenzip_single_fast.c" -o "%OBJ%\v35-x86-debug-sevenzip.obj"
if errorlevel 1 exit /b 1
"%NASM%" -f win32 -O9 -D LZMA_C_API_DEBUG=1 "%LZMA_SRC%\lzma_static_x86.asm" -o "%OBJ%\v35-x86-debug-lzma.obj"
if errorlevel 1 exit /b 1
"%CLANG%" %FLAGS% -I"%LZMA_SRC%" -c "%LZMA_SRC%\sevenzip_single_fast.c" -o "%OBJ%\v35-x86-release-test-sevenzip.obj"
if errorlevel 1 exit /b 1
"%NASM%" -f win32 -O9 "%LZMA_SRC%\lzma_static_x86.asm" -o "%OBJ%\v35-x86-release-test-lzma.obj"
if errorlevel 1 exit /b 1
"%LLD%" /nologo /NODEFAULTLIB /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup /OPT:REF /OPT:ICF /INCREMENTAL:NO /DYNAMICBASE:NO /FIXED /MACHINE:X86 /MERGE:.rdata=.text /MERGE:.data=.text /MERGE:.idata=.text /SECTION:.text,ERW /MAP:"%MAP%\v35-x86-debug.map" /OUT:"%TEST%\smolnes-v3.5-7z-debug.exe" "%OBJ%\v35-x86-debug-main.obj" "%OBJ%\v35-x86-debug-apu.obj" "%OBJ%\v35-x86-debug-audio.obj" "%OBJ%\v35-x86-debug-sevenzip.obj" "%OBJ%\v35-x86-debug-lzma.obj" "%MALLOC_LIB%" kernel32.lib user32.lib gdi32.lib comdlg32.lib winmm.lib libcmt.lib
if errorlevel 1 exit /b 1
"%LLD%" /nologo /NODEFAULTLIB /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup /OPT:REF /OPT:ICF /INCREMENTAL:NO /DYNAMICBASE:NO /FIXED /MACHINE:X86 /MERGE:.rdata=.text /MERGE:.data=.text /MERGE:.idata=.text /SECTION:.text,ERW /MAP:"%MAP%\v35-x86-release-test.map" /OUT:"%TEST%\smolnes-v3.5-7z-release-test.exe" "%OBJ%\v35-x86-release-test-main.obj" "%OBJ%\v35-x86-debug-apu.obj" "%OBJ%\v35-x86-debug-audio.obj" "%OBJ%\v35-x86-release-test-sevenzip.obj" "%OBJ%\v35-x86-release-test-lzma.obj" "%MALLOC_LIB%" kernel32.lib user32.lib gdi32.lib comdlg32.lib winmm.lib libcmt.lib
if errorlevel 1 exit /b 1
endlocal
