@echo off
setlocal

echo ================================
echo   Cleaning CMake build folders
echo ================================
echo.

REM Always run from repo root (directory where this script lives)
cd /d "%~dp0"

REM ------------------------------------------
REM Remove main build folders
REM ------------------------------------------

for %%D in (build build_game) do (
    if exist "%%D" (
        echo Removing %%D ...
        rmdir /S /Q "%%D"
    )
)

REM ------------------------------------------
REM Remove in-source CMake artifacts (if present)
REM ------------------------------------------

for %%F in (
    CMakeCache.txt
    cmake_install.cmake
    CPackConfig.cmake
    CPackSourceConfig.cmake
    giraphics.sln
) do (
    if exist "%%F" (
        echo Removing %%F ...
        del /F /Q "%%F"
    )
)

if exist "CMakeFiles" (
    echo Removing CMakeFiles ...
    rmdir /S /Q "CMakeFiles"
)

echo.
echo Done!
echo.

endlocal
pause
