@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "GAME_NAME=%~1"

echo ================================
echo   Cleaning CMake build folders
echo ================================
echo.

REM Always run from repo root (directory where this script lives)
cd /d "%~dp0"

if "%GAME_NAME%"=="" goto :clean_all

set "BUILD_SUFFIX=%GAME_NAME: =_%"
set "FOUND_ANY=0"

echo Targeted clean for project: %GAME_NAME%
echo.

call :remove_dir "build_%BUILD_SUFFIX%_editor"
call :remove_dir "build_%BUILD_SUFFIX%_game"

call :clean_web_preset "build\web-debug"
call :clean_web_preset "build\web-release-split"

if "!FOUND_ANY!"=="0" (
    echo No build artifacts were found for %GAME_NAME%.
)

goto :done

:clean_all
echo Cleaning all project build folders.
echo.

for %%D in (build build_game) do (
    if exist "%%D" (
        echo Removing %%D ...
        rmdir /S /Q "%%D"
    )
)

for /D %%D in (build_*) do (
    if exist "%%D" (
        echo Removing %%D ...
        rmdir /S /Q "%%D"
    )
)

REM Remove in-source CMake artifacts (if present)
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

goto :done

:clean_web_preset
set "WEB_DIR=%~1"
if not exist "%WEB_DIR%" goto :eof

set "CACHE_FILE=%WEB_DIR%\CMakeCache.txt"
set "CACHED_GAME="
if exist "%CACHE_FILE%" (
    for /f "tokens=2 delims==" %%I in ('findstr /b /c:"SOFASPUDS_GAME_NAME:STRING=" "%CACHE_FILE%" 2^>nul') do (
        set "CACHED_GAME=%%I"
    )
)

if defined CACHED_GAME if /I "!CACHED_GAME!"=="%GAME_NAME%" (
    echo Removing !WEB_DIR! because it is configured for %GAME_NAME% ...
    rmdir /S /Q "!WEB_DIR!"
    set "FOUND_ANY=1"
    goto :eof
)

call :remove_web_artifact "%WEB_DIR%\Sandbox\%GAME_NAME%.html"
call :remove_web_artifact "%WEB_DIR%\Sandbox\%GAME_NAME%.js"
call :remove_web_artifact "%WEB_DIR%\Sandbox\%GAME_NAME%.wasm"
call :remove_web_artifact "%WEB_DIR%\Sandbox\%GAME_NAME%.data"
call :remove_web_artifact "%WEB_DIR%\Sandbox\%GAME_NAME%.project_root.txt"

goto :eof

:remove_dir
if exist "%~1" (
    echo Removing %~1 ...
    rmdir /S /Q "%~1"
    set "FOUND_ANY=1"
)
goto :eof

:remove_web_artifact
if exist "%~1" (
    echo Removing %~1 ...
    del /F /Q "%~1"
    set "FOUND_ANY=1"
)
goto :eof

:done
echo.
echo Done!
echo.
echo Usage:
echo   clean.bat          ^(remove all build folders^)
echo   clean.bat NewGame2 ^(remove only NewGame2-related build artifacts^)
echo.

endlocal
pause
