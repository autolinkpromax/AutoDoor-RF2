@echo off
chcp 65001 >nul
setlocal
echo กำลังสร้าง Shortcut บน Desktop...

set "TARGET_BAT=%~dp0One-Click-Flash-ESP32C3.bat"
set "SHORTCUT_PATH=%USERPROFILE%\Desktop\AutoDoor ESP32-C3 Flasher.lnk"

powershell -NoProfile -Command "$ws = New-Object -ComObject WScript.Shell; $s = $ws.CreateShortcut('%SHORTCUT_PATH%'); $s.TargetPath = '%TARGET_BAT%'; $s.WorkingDirectory = '%~dp0'; $s.Description = 'AutoDoor ESP32-C3 One-Click Firmware Flasher'; $s.Save()"

if exist "%SHORTCUT_PATH%" (
    echo.
    echo =====================================================================
    echo [สำเร็จ] สร้าง Shortcut บน Desktop เรียบร้อยแล้ว:
    echo "%SHORTCUT_PATH%"
    echo =====================================================================
) else (
    echo.
    echo [ข้อผิดพลาด] ไม่สามารถสร้าง Shortcut ได้
)
echo.
pause
