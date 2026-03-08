@echo off
setlocal

set "PRESET=web-release-split"
set "GAME_NAME=BloodyGoodCurry"
set "FORCE_CONFIGURE=0"
set "AUTO_SERVE=0"

if "%~1"=="" if "%~2"=="" if "%~3"=="" (
    set "FORCE_CONFIGURE=1"
    set "AUTO_SERVE=1"
)

for %%A in ("%~1" "%~2" "%~3") do (
    if /I "%%~A"=="debug" set "PRESET=web-debug"
    if /I "%%~A"=="fast" set "PRESET=web-debug"
    if /I "%%~A"=="dev" set "PRESET=web-debug"
    if /I "%%~A"=="release" set "PRESET=web-release-split"
    if /I "%%~A"=="release-split" set "PRESET=web-release-split"
    if /I "%%~A"=="releasefast" set "PRESET=web-release-split"
    if /I "%%~A"=="reconfigure" set "FORCE_CONFIGURE=1"
)

for %%A in ("%~1" "%~2" "%~3") do (
    if not "%%~A"=="" (
        if /I not "%%~A"=="debug" if /I not "%%~A"=="fast" if /I not "%%~A"=="dev" if /I not "%%~A"=="release" if /I not "%%~A"=="release-split" if /I not "%%~A"=="releasefast" if /I not "%%~A"=="reconfigure" (
            set "GAME_NAME=%%~A"
        )
    )
)

echo === SofaSpuds Web HTML Build ===
echo Preset: %PRESET%
echo Game: %GAME_NAME%

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

if "%AUTO_SERVE%"=="1" (
    where python >nul 2>nul
    if errorlevel 1 (
        echo [ERROR] Python is not installed or not in PATH.
        echo Python is required to start the local web server automatically.
        exit /b 1
    )
)

set "BUILD_DIR=%~dp0build\%PRESET%"
if not exist "%BUILD_DIR%\CMakeCache.txt" set "FORCE_CONFIGURE=1"

echo.
if "%FORCE_CONFIGURE%"=="1" (
    echo [1/2] Configuring CMake...
    cmake --preset %PRESET% -DSOFASPUDS_GAME_NAME=%GAME_NAME%
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

set "HTML_FILE=%~dp0build\%PRESET%\Sandbox\%GAME_NAME%.html"
if not exist "%HTML_FILE%" set "HTML_FILE=%~dp0build\%PRESET%\%GAME_NAME%.html"
echo.
if exist "%HTML_FILE%" (
    echo [OK] HTML generated:
    echo      %HTML_FILE%
) else (
    echo [WARN] Build succeeded but HTML file was not found at:
    echo       %HTML_FILE%
)

if "%AUTO_SERVE%"=="1" (
    echo.
    echo [3/3] Starting local web server on http://localhost:8000/
    echo Press Ctrl+C to stop the server.
    python -m http.server 8000 -d build\web-release-split\Sandbox
    exit /b %ERRORLEVEL%
)

echo.
echo Usage:
echo   build_web_html.bat ^(defaults to BloodyGoodCurry + release-split + reconfigure, then starts server^)
echo   build_web_html.bat debug
echo   build_web_html.bat fast
echo   build_web_html.bat dev
echo   build_web_html.bat release
echo   build_web_html.bat release-split
echo   build_web_html.bat NewGame
echo   build_web_html.bat BloodyGoodCurry release-split reconfigure
echo   build_web_html.bat release-split NewGame reconfigure

endlocal
exit /b 0
