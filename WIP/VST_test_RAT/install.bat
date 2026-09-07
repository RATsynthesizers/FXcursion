@echo off
rem =============================================================================
rem  Copy the built plugin into the shared VST3 folder.
rem
rem  NEEDS ADMINISTRATOR - C:\Program Files\Common Files\VST3 is not writable by
rem  a normal user. Right click -> Run as administrator.
rem =============================================================================

setlocal
cd /d "%~dp0"

set "SRC=%~dp0build\VST_test_RAT_artefacts\Release\VST3\FXcursion FX Bench.vst3"
set "DST=C:\Program Files\Common Files\VST3\FXcursion FX Bench.vst3"

if not exist "%SRC%" (
    echo Not built yet. Run build.bat first.
    exit /b 1
)

rem A plain write test, because xcopy's own failure message is easy to miss.
copy /y nul "C:\Program Files\Common Files\VST3\.fxbench_write_test" >nul 2>&1
if errorlevel 1 (
    echo.
    echo Cannot write to C:\Program Files\Common Files\VST3
    echo Right click install.bat and choose "Run as administrator".
    echo.
    exit /b 1
)
del "C:\Program Files\Common Files\VST3\.fxbench_write_test" >nul 2>&1

echo Installing to %DST%

rem A VST3 on Windows is a folder, not a file, so this copies the whole bundle.
if exist "%DST%" rmdir /s /q "%DST%"
xcopy /e /i /y /q "%SRC%" "%DST%" >nul || exit /b 1

echo.
echo Done.
echo.
echo In FL Studio: Options -^> Manage plugins -^> Find more plugins.
echo The plugin appears as "FXcursion FX Bench" by RAT Synthesizers.
echo.
echo Set the project sample rate to 48000 Hz - the effect code has 48 kHz
echo compiled into it and everything is out by the ratio at any other rate.
endlocal
