@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion
title AutoDoor ESP32-C3 One-Click Flasher

set "PYTHONIOENCODING=utf-8"
set "PYTHONUTF8=1"
set "PY_CMD="

:: 1. Check system python
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

:: 3. Check Python 3.13 default path
if exist "C:\Python313\python.exe" (
    set "PY_CMD=C:\Python313\python.exe"
    goto :RUN
)

:: 4. Check PlatformIO Python virtualenv
if exist "%USERPROFILE%\.platformio\penv\Scripts\python.exe" (
    set "PY_CMD=%USERPROFILE%\.platformio\penv\Scripts\python.exe"
    goto :RUN
)

echo =====================================================================
echo [ข้อผิดพลาด] ไม่พบ Python ในระบบ
echo กรุณาติดตั้ง Python 3 (https://www.python.org/) หรือรันผ่าน PlatformIO
echo =====================================================================
echo.
pause
exit /b 1

:RUN
start "" "%PY_CMD%" "%~dp01-Click-Flash-ESP32C3\flasher.py" --gui
exit /b 0
