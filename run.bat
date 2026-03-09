@echo off
setlocal

set "GAME_NAME=%~1"
if "%GAME_NAME%"=="" set "GAME_NAME=BloodyGoodCurry"
set "BUILD_SUFFIX=%GAME_NAME: =_%"
set "BUILD_DIR=build_%BUILD_SUFFIX%_editor"
set "ROOT_DIR=%~dp0"
if "%ROOT_DIR:~-1%"=="\" set "ROOT_DIR=%ROOT_DIR:~0,-1%"
set "ROOT_DIR_FWD=%ROOT_DIR:\=/%"
set "CMAKE_ARGS=-G "Visual Studio 17 2022" -A x64 -DSOFASPUDS_ENABLE_EDITOR=ON -DSOFASPUDS_GAME_NAME=%GAME_NAME% .."

if not exist "%BUILD_DIR%" (
    mkdir "%BUILD_DIR%"
)

if exist "%BUILD_DIR%\CMakeCache.txt" (
    findstr /C:"CMAKE_HOME_DIRECTORY:INTERNAL=%ROOT_DIR_FWD%" "%BUILD_DIR%\CMakeCache.txt" >nul
    if errorlevel 1 (
        echo Detected a stale CMake cache in %BUILD_DIR%.
        echo Deleting the old build folder so it can be regenerated for this machine...
        rmdir /S /Q "%BUILD_DIR%"
        mkdir "%BUILD_DIR%"
    )
)

pushd "%BUILD_DIR%"
cmake %CMAKE_ARGS%
if errorlevel 1 (
    popd
    echo Initial CMake configure failed. Retrying with a clean build folder...
    rmdir /S /Q "%BUILD_DIR%"
    mkdir "%BUILD_DIR%"
    pushd "%BUILD_DIR%"
    cmake %CMAKE_ARGS%
    if errorlevel 1 (
        popd
        echo CMake configure failed.
        endlocal
        pause
        exit /b 1
    )
)
popd

echo Configured editor build for %GAME_NAME% in %BUILD_DIR%.

endlocal
pause
