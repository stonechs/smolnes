@echo off
setlocal
set "DIST=%~dp0..\..\"
set "SRC=%DIST%source"
set "TEST=%~dp0"
set "OUT=%TEMP%\smolnes-v3.3-headless-tests"
if not exist "%OUT%" mkdir "%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x86 -host_arch=x64 >nul
if errorlevel 1 exit /b 1
cl /nologo /O1 /TC /I"%SRC%" /Fo:"%OUT%\\" "%TEST%dmc_apu_test.c" "%SRC%\apu.c" /Fe:"%OUT%\dmc_apu_test.exe"
if errorlevel 1 exit /b 1
"%OUT%\dmc_apu_test.exe"
if errorlevel 1 exit /b 1
cl /nologo /O1 /TC /D AUDIO_TESTING /I"%SRC%" /Fo:"%OUT%\\" "%TEST%audio_dmc_mix_test.c" "%SRC%\audio_win32.c" /Fe:"%OUT%\audio_dmc_mix_test.exe" /link winmm.lib
if errorlevel 1 exit /b 1
"%OUT%\audio_dmc_mix_test.exe"
if errorlevel 1 exit /b 1
endlocal
