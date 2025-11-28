@echo off
setlocal

REM OPTIONAL: Nuke build_game to reset all cached options
if exist build_game (
    echo Removing build_game to reset CMake options...
    rmdir /S /Q build_game
)

mkdir build_game

pushd build_game
cmake -DSOFASPUDS_ENABLE_EDITOR=ON ..
popd

endlocal
pause
