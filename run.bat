@echo off
setlocal

set "GAME_NAME=%~1"
if "%GAME_NAME%"=="" set "GAME_NAME=BloodyGoodCurry"
set "BUILD_SUFFIX=%GAME_NAME: =_%"
set "BUILD_DIR=build_%BUILD_SUFFIX%_editor"

if not exist "%BUILD_DIR%" (
    mkdir "%BUILD_DIR%"
)

pushd "%BUILD_DIR%"
cmake -DSOFASPUDS_ENABLE_EDITOR=ON -DSOFASPUDS_GAME_NAME=%GAME_NAME% ..
popd

echo Configured editor build for %GAME_NAME% in %BUILD_DIR%.

endlocal
pause
