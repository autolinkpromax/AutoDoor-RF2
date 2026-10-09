@echo off
setlocal enabledelayedexpansion
title AutoDoor ESP32-C3 Flasher GUI

set "PY_CMD="

:: 1. Check python in PATH
where python >nul 2>&1
if %ERRORLEVEL% equ 0 (
    set "PY_CMD=python"
    goto :RUN
)

:: 2. Check py launcher
where py >nul 2>&1
if %ERRORLEVEL% equ 0 (
    set "PY_CMD=py"
    goto :RUN
)

:: 3. Check PlatformIO Python
if exist "%USERPROFILE%\.platformio\penv\Scripts\python.exe" (
    set "PY_CMD=%USERPROFILE%\.platformio\penv\Scripts\python.exe"
    goto :RUN
)

echo =====================================================================
echo [ERROR] Python not found in system PATH or PlatformIO.
echo Please install Python 3 (https://www.python.org/) or run via PlatformIO.
echo =====================================================================
echo.
pause
exit /b 1

:RUN
start "" "%PY_CMD%" "%~dp0flasher.py" --gui
