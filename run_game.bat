@echo off
setlocal

set "GAME_NAME=%~1"
if "%GAME_NAME%"=="" set "GAME_NAME=BloodyGoodCurry"
set "BUILD_SUFFIX=%GAME_NAME: =_%"
set "BUILD_DIR=build_%BUILD_SUFFIX%_game"

if not exist "%BUILD_DIR%" (
    mkdir "%BUILD_DIR%"
)

echo === Running CMake with editor DISABLED ===
pushd "%BUILD_DIR%"
cmake -DSOFASPUDS_ENABLE_EDITOR=OFF -DSOFASPUDS_GAME_NAME=%GAME_NAME% ..
popd

echo === Done. Open %BUILD_DIR% and build %GAME_NAME%. ===
pause
endlocal
