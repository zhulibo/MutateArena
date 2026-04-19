@echo off
REM Set console to UTF-8 to prevent character encoding issues
chcp 65001 >nul
setlocal

set "SOURCE_DIR=%~dp0"
set "TEMP_FILE=C:\Users\yoyoy\Desktop\Temp\MutateArena_temp.zip"
set "DEST_FILE=C:\Users\yoyoy\OneDrive\Backup\MutateArena\MutateArena.zip"

set "EXCLUDE_ITEMS=.git, .idea, Binaries, Build, DerivedDataCache, Intermediate, Saved"

echo.
powershell -Command "Write-Host 'Skip %EXCLUDE_ITEMS%' -ForegroundColor Cyan"
echo.

echo Starting backup...

REM Use try-catch to ensure exit code 1 on failure (Single-line to prevent Batch parsing errors)
powershell -NoProfile -Command "$ErrorActionPreference = 'Stop'; try { $exclude = '%EXCLUDE_ITEMS%' -split ',' | ForEach-Object { $_.Trim() }; Get-ChildItem -Path '%SOURCE_DIR%' -Exclude $exclude | Compress-Archive -DestinationPath '%TEMP_FILE%' -Force } catch { exit 1 }"
    
REM Check the exit code of the PowerShell command
if %errorlevel% equ 0 (
    REM Replace the old backup only if the compression succeeded
    move /y "%TEMP_FILE%" "%DEST_FILE%" >nul
    echo.
    
    REM Print success message in Green
    powershell -Command "Write-Host 'Backup successful!' -ForegroundColor Green"
    echo.
    
    REM Close automatically after 3 seconds on success
    timeout /t 5
    exit /b
) else (
    REM Clean up the corrupted temporary file if it exists
    if exist "%TEMP_FILE%" del "%TEMP_FILE%"
    echo.
    
    REM Print error message in Red
    powershell -Command "Write-Host 'Backup failed! (Please check if Unreal Editor is closed)' -ForegroundColor Red"
    echo.
    
    REM Pause and keep the window open on failure
    pause
    exit /b %errorlevel%
)
