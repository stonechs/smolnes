@echo off
setlocal
for %%I in ("%~dp0..\..\..") do set "ROOT=%%~fI"
set "DIST=%~dp0"
set "SRC=%DIST%source"
set "OBJ=%DIST%build-support\objects"
set "MAP=%DIST%build-support\maps"
set "REL=%DIST%release"
set "CLANG=C:\Users\maco\AppData\Local\Programs\LLVM\bin\clang.exe"
set "LLD=C:\Users\maco\AppData\Local\Programs\LLVM\bin\lld-link.exe"
set "CRINKLER=%ROOT%\tools\crinkler\crinkler30b\Win32\Crinkler.exe"
set "UPXBIN=%ROOT%\tools\upx.exe"

call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x86 -host_arch=x64 >nul
if errorlevel 1 exit /b 1
"%CLANG%" --target=i686-pc-windows-msvc -Oz -fms-extensions -fms-compatibility -fms-compatibility-version=19.44 -fno-stack-protector -fno-unwind-tables -fno-asynchronous-unwind-tables -ffunction-sections -fdata-sections -DNDEBUG -c "%SRC%\deobfuscated_win32.c" -o "%OBJ%\v32-x86-main.obj"
if errorlevel 1 exit /b 1
"%CLANG%" --target=i686-pc-windows-msvc -Oz -fms-extensions -fms-compatibility -fms-compatibility-version=19.44 -fno-stack-protector -fno-unwind-tables -fno-asynchronous-unwind-tables -ffunction-sections -fdata-sections -DNDEBUG -c "%SRC%\apu.c" -o "%OBJ%\v32-x86-apu.obj"
if errorlevel 1 exit /b 1
"%CLANG%" --target=i686-pc-windows-msvc -Oz -fms-extensions -fms-compatibility -fms-compatibility-version=19.44 -fno-stack-protector -fno-unwind-tables -fno-asynchronous-unwind-tables -ffunction-sections -fdata-sections -DNDEBUG -c "%SRC%\audio_win32.c" -o "%OBJ%\v32-x86-audio.obj"
if errorlevel 1 exit /b 1

if exist "%REL%\smolnes-v3.2-clang-x86.exe" del /q "%REL%\smolnes-v3.2-clang-x86.exe"
"%LLD%" /nologo /NODEFAULTLIB /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup /OPT:REF /OPT:ICF /INCREMENTAL:NO /DYNAMICBASE:NO /FIXED /MACHINE:X86 /MERGE:.rdata=.text /MERGE:.data=.text /MERGE:.idata=.text /SECTION:.text,ERW /MAP:"%MAP%\v32-x86-lld.map" /OUT:"%REL%\smolnes-v3.2-clang-x86.exe" "%OBJ%\v32-x86-main.obj" "%OBJ%\v32-x86-apu.obj" "%OBJ%\v32-x86-audio.obj" kernel32.lib user32.lib gdi32.lib comdlg32.lib winmm.lib libcmt.lib
if errorlevel 1 exit /b 1

if exist "%REL%\smolnes-v3.2-clang-x86-crinkler.exe" del /q "%REL%\smolnes-v3.2-clang-x86-crinkler.exe"
"%CRINKLER%" /NODEFAULTLIB /ENTRY:mainCRTStartup /SUBSYSTEM:WINDOWS /COMPMODE:FAST /HASHSIZE:32 /HASHTRIES:100 /TRANSFORM:CALLS /OUT:"%REL%\smolnes-v3.2-clang-x86-crinkler.exe" "%OBJ%\v32-x86-main.obj" "%OBJ%\v32-x86-apu.obj" "%OBJ%\v32-x86-audio.obj" kernel32.lib user32.lib gdi32.lib comdlg32.lib winmm.lib libcmt.lib
if errorlevel 1 exit /b 1

call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul
if errorlevel 1 exit /b 1
"%CLANG%" --target=x86_64-pc-windows-msvc -Oz -flto -fms-extensions -fms-compatibility -fms-compatibility-version=19.44 -fno-stack-protector -fno-unwind-tables -fno-asynchronous-unwind-tables -ffunction-sections -fdata-sections -DNDEBUG -c "%SRC%\deobfuscated_win32.c" -o "%OBJ%\v32-x64-main.obj"
if errorlevel 1 exit /b 1
"%CLANG%" --target=x86_64-pc-windows-msvc -Oz -flto -fms-extensions -fms-compatibility -fms-compatibility-version=19.44 -fno-stack-protector -fno-unwind-tables -fno-asynchronous-unwind-tables -ffunction-sections -fdata-sections -DNDEBUG -c "%SRC%\apu.c" -o "%OBJ%\v32-x64-apu.obj"
if errorlevel 1 exit /b 1
"%CLANG%" --target=x86_64-pc-windows-msvc -Oz -flto -fms-extensions -fms-compatibility -fms-compatibility-version=19.44 -fno-stack-protector -fno-unwind-tables -fno-asynchronous-unwind-tables -ffunction-sections -fdata-sections -DNDEBUG -c "%SRC%\audio_win32.c" -o "%OBJ%\v32-x64-audio.obj"
if errorlevel 1 exit /b 1
if exist "%REL%\smolnes-v3.2-clang-x64.exe" del /q "%REL%\smolnes-v3.2-clang-x64.exe"
"%LLD%" /nologo /NODEFAULTLIB /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup /OPT:REF /OPT:ICF /INCREMENTAL:NO /DYNAMICBASE:NO /FIXED /MACHINE:X64 /MAP:"%MAP%\v32-x64-lld.map" /OUT:"%REL%\smolnes-v3.2-clang-x64.exe" "%OBJ%\v32-x64-main.obj" "%OBJ%\v32-x64-apu.obj" "%OBJ%\v32-x64-audio.obj" kernel32.lib user32.lib gdi32.lib comdlg32.lib winmm.lib libcmt.lib
if errorlevel 1 exit /b 1

if exist "%REL%\smolnes-v3.2-clang-x64-upx.exe" del /q "%REL%\smolnes-v3.2-clang-x64-upx.exe"
"%UPXBIN%" --brute -o "%REL%\smolnes-v3.2-clang-x64-upx.exe" "%REL%\smolnes-v3.2-clang-x64.exe"
if errorlevel 1 exit /b 1
"%UPXBIN%" -t "%REL%\smolnes-v3.2-clang-x64-upx.exe"
if errorlevel 1 exit /b 1
