@echo off
setlocal

echo === Cleaning build_game (editor OFF) ===
if exist build_game (
    rmdir /S /Q build_game
)

mkdir build_game

echo === Running CMake with editor DISABLED ===
pushd build_game
cmake -DSOFASPUDS_ENABLE_EDITOR=OFF ..
popd

echo === Done. Open build_game/MyEngine.sln and build. ===
pause
endlocal
