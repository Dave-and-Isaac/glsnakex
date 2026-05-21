@echo off
echo.
echo  GLSnakeX - RXGL Path Setup
echo  ===========================
echo.
echo  This sets the RXGL_DIR environment variable so Visual Studio
echo  can find the RXGL source files when building.
echo.
echo  Default (press Enter): %~dp0..\RXGL\
echo  Use this if you cloned RXGL next to this repo.
echo.
set /p RXGL_PATH="  RXGL path (or Enter for default): "
if "%RXGL_PATH%"=="" set "RXGL_PATH=%~dp0..\"
if "%RXGL_PATH:~-1%"=="\" set "RXGL_PATH=%RXGL_PATH:~0,-1%"
echo.
setx RXGL_DIR "%RXGL_PATH%"
echo.
echo  Done! Restart Visual Studio then open GLSnake-Xbox.sln and build.
echo.
pause
