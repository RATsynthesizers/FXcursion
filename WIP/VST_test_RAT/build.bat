@echo off
rem =============================================================================
rem  Build the FX Bench plugin and prove it loads.
rem
rem  No admin needed. Run install.bat afterwards (as administrator) to put it
rem  where FL Studio looks.
rem =============================================================================

setlocal
cd /d "%~dp0"

if not exist "_deps\JUCE\CMakeLists.txt" (
    echo.
    echo JUCE is missing. From this folder:
    echo   git clone --depth 1 --branch 8.0.4 https://github.com/juce-framework/JUCE.git _deps\JUCE
    echo.
    exit /b 1
)

echo === configuring ===
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 || exit /b 1

echo.
echo === building ===
cmake --build build --config Release || exit /b 1

echo.
echo === load test ===
rem Compiling and loading are different claims. This opens the plugin the way a
rem DAW does and pushes noise through every effect.
"build\FxBenchLoadTest_artefacts\Release\FxBenchLoadTest.exe" ^
    "%~dp0build\VST_test_RAT_artefacts\Release\VST3\FXcursion FX Bench.vst3" || exit /b 1

echo.
echo Built: build\VST_test_RAT_artefacts\Release\VST3\FXcursion FX Bench.vst3
echo Now run install.bat as administrator.
endlocal
