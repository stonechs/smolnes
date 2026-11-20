@echo off
setlocal
for %%I in ("%~dp0..\..\..") do set "ROOT=%%~fI"
set CAND=%ROOT%\dist\V1.6\candidate
set CLANG=%ROOT%\tools\clang\llvm-minimal\clang+llvm-23.1.5-x86_64-pc-windows-msvc\bin\clang.exe
set LLD=%ROOT%\tools\clang\llvm-minimal\clang+llvm-23.1.5-x86_64-pc-windows-msvc\bin\lld-link.exe
set CRINKLER=%ROOT%\tools\crinkler\crinkler30b\Win32\Crinkler.exe
set UPXBIN=%ROOT%\tools\upx.exe
set SRC=%CAND%\source\deobfuscated_win32.c
set OBJ=%CAND%\build-support\objects
set INT=%CAND%\build-support\intermediate
set REL=%CAND%\release

call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x86 -host_arch=x64 >nul
if errorlevel 1 exit /b 1
"%CLANG%" --target=i686-pc-windows-msvc -Oz -fms-extensions -fms-compatibility -fms-compatibility-version=19.44 -fno-stack-protector -fno-unwind-tables -fno-asynchronous-unwind-tables -ffunction-sections -fdata-sections -DNDEBUG -DSMOLNES_OMIT_TIMER_FAILURE_CHECKS -DSMOLNES_OMIT_TIMER_SHUTDOWN_CLEANUP -DSMOLNES_NO_ERROR_CHECKS -c "%SRC%" -o "%OBJ%\smolnes-v1.6-clang-x86.obj"
if errorlevel 1 exit /b 1
"%LLD%" /nologo /NODEFAULTLIB /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup /OPT:REF /OPT:ICF /INCREMENTAL:NO /DYNAMICBASE:NO /FIXED /MACHINE:X86 /MERGE:.rdata=.text /MERGE:.data=.text /MERGE:.idata=.text /SECTION:.text,ERW /MAP:"%CAND%\build-support\maps\link-x86-raw.map" /OUT:"%REL%\smolnes-v1.6-clang-x86.exe" "%OBJ%\smolnes-v1.6-clang-x86.obj" kernel32.lib user32.lib gdi32.lib comdlg32.lib
if errorlevel 1 exit /b 1
"%CRINKLER%" /NODEFAULTLIB /ENTRY:mainCRTStartup /SUBSYSTEM:WINDOWS /COMPMODE:FAST /HASHSIZE:32 /HASHTRIES:100 /TRANSFORM:CALLS /OUT:"%REL%\smolnes-v1.6-clang-x86-crinkler.exe" "%OBJ%\smolnes-v1.6-clang-x86.obj" kernel32.lib user32.lib gdi32.lib comdlg32.lib
if errorlevel 1 exit /b 1

call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul
if errorlevel 1 exit /b 1
"%CLANG%" --target=x86_64-pc-windows-msvc -Oz -flto -fms-extensions -fms-compatibility -fms-compatibility-version=19.44 -fno-stack-protector -fno-unwind-tables -fno-asynchronous-unwind-tables -ffunction-sections -fdata-sections -DNDEBUG -DSMOLNES_OMIT_TIMER_FAILURE_CHECKS -DSMOLNES_OMIT_TIMER_SHUTDOWN_CLEANUP -DSMOLNES_NO_ERROR_CHECKS -c "%SRC%" -o "%OBJ%\smolnes-v1.6-clang-x64-lto.obj"
if errorlevel 1 exit /b 1
"%LLD%" /nologo /NODEFAULTLIB /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup /OPT:REF /OPT:ICF /INCREMENTAL:NO /DYNAMICBASE:NO /FIXED /MACHINE:X64 /MAP:"%CAND%\build-support\maps\link-x64.map" /OUT:"%REL%\smolnes-v1.6-clang-x64.exe" "%OBJ%\smolnes-v1.6-clang-x64-lto.obj" kernel32.lib user32.lib gdi32.lib comdlg32.lib
if errorlevel 1 exit /b 1
if exist "%REL%\smolnes-v1.6-clang-x64-upx.exe" del /q "%REL%\smolnes-v1.6-clang-x64-upx.exe"
"%UPXBIN%" --brute -o "%REL%\smolnes-v1.6-clang-x64-upx.exe" "%REL%\smolnes-v1.6-clang-x64.exe"
if errorlevel 1 exit /b 1
