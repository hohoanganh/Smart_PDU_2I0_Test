@echo off
REM Khoi tao git cho Smart PDU 2I0 - chay 1 lan duy nhat
cd /d "%~dp0"

where git >nul 2>nul
if errorlevel 1 (
    echo [LOI] Chua cai Git. Tai tai: https://git-scm.com/download/win
    pause
    exit /b 1
)

if exist .git (
    echo [!] Da co .git - bo qua buoc init.
) else (
    git init -b main
)

git config user.name "Hoang Anh"
git config user.email "hohoanga@gmail.com"

git add -A
git commit -m "Initial commit: Smart PDU 2I0 test FW v0.1.0" -m "4 relay chot qua PCA9554, 4 nut + 4 LED, CLI day du (ke thua AK MCU KIT)"

echo.
git log --oneline
pause
