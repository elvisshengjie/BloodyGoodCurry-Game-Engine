@echo off
setlocal

set "PRESET=web-release"
set "FORCE_CONFIGURE=0"
if /I "%~1"=="debug" set "PRESET=web-debug"
if /I "%~1"=="fast" set "PRESET=web-debug"
if /I "%~1"=="dev" set "PRESET=web-debug"
if /I "%~1"=="release" set "PRESET=web-release"
if /I "%~1"=="release-split" set "PRESET=web-release-split"
if /I "%~1"=="releasefast" set "PRESET=web-release-split"
if /I "%~1"=="reconfigure" set "FORCE_CONFIGURE=1"
if /I "%~2"=="reconfigure" set "FORCE_CONFIGURE=1"

echo === SofaSpuds Web HTML Build ===
echo Preset: %PRESET%

if not defined EMSDK (
    if exist "%USERPROFILE%\emsdk\emsdk_env.bat" (
        echo EMSDK not set. Loading "%USERPROFILE%\emsdk\emsdk_env.bat"...
        call "%USERPROFILE%\emsdk\emsdk_env.bat"
    )
)

if not defined EMSDK (
    echo [ERROR] EMSDK is not set.
    echo Please install/activate emsdk first, then run again.
    echo Example:
    echo   call %%USERPROFILE%%\emsdk\emsdk_env.bat
    exit /b 1
)

where ninja >nul 2>nul
if errorlevel 1 (
    echo [ERROR] Ninja is not installed or not in PATH.
    echo Install Ninja, then run this script again.
    echo Example:
    echo   winget install Ninja-build.Ninja
    echo If winget is unavailable, download from:
    echo   https://github.com/ninja-build/ninja/releases
    exit /b 1
)

set "BUILD_DIR=%~dp0build\%PRESET%"
if not exist "%BUILD_DIR%\CMakeCache.txt" set "FORCE_CONFIGURE=1"

echo.
if "%FORCE_CONFIGURE%"=="1" (
    echo [1/2] Configuring CMake...
    cmake --preset %PRESET%
    if errorlevel 1 (
        echo [ERROR] CMake configure failed.
        exit /b 1
    )
) else (
    echo [1/2] Configure step skipped ^(cache exists^). Use "reconfigure" to force.
)

echo.
echo [2/2] Building...
cmake --build --preset %PRESET%
if errorlevel 1 (
    echo [ERROR] Build failed.
    exit /b 1
)

set "HTML_FILE=%~dp0build\%PRESET%\Sandbox\BloodyGoodCurry.html"
if not exist "%HTML_FILE%" set "HTML_FILE=%~dp0build\%PRESET%\BloodyGoodCurry.html"
echo.
if exist "%HTML_FILE%" (
    echo [OK] HTML generated:
    echo      %HTML_FILE%
) else (
    echo [WARN] Build succeeded but HTML file was not found at:
    echo       %HTML_FILE%
)

echo.
echo Usage:
echo   build_web_html.bat          ^(release^)
echo   build_web_html.bat debug
echo   build_web_html.bat fast     ^(alias of debug, faster link^)
echo   build_web_html.bat dev      ^(alias of debug, faster link^)
echo   build_web_html.bat release
echo   build_web_html.bat release-split  ^(release opt, split files, faster load^)
echo   build_web_html.bat [mode] reconfigure

endlocal
exit /b 0
