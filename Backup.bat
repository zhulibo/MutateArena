@echo off
setlocal

set "SOURCE_DIR=%~dp0"
set "DEST_FILE=C:\Users\yoyoy\OneDrive\Backup\MutateArena\MutateArena.zip"
set "TEMP_FILE=C:\Users\yoyoy\Desktop\Temp\MutateArena_temp.zip"

set "EXCLUDE_ITEMS='.git', '.idea', 'Binaries', 'Build', 'DerivedDataCache', 'Intermediate', 'Saved'"

echo Starting backup...

:: Use try-catch to ensure exit code 1 on failure, and compress to a temporary file
powershell -NoProfile -Command "$ErrorActionPreference = 'Stop'; try { Get-ChildItem -Path '%SOURCE_DIR%' -Exclude %EXCLUDE_ITEMS% | Compress-Archive -DestinationPath '%TEMP_FILE%' -Force } catch { exit 1 }"

:: Check the exit code of the PowerShell command
if %errorlevel% equ 0 (
    :: Replace the old backup only if the compression succeeded
    move /y "%TEMP_FILE%" "%DEST_FILE%" >nul
    echo.
    :: Print success message in Green
    powershell -Command "Write-Host 'Backup successful!' -ForegroundColor Green"
    echo.
    :: Close automatically after 3 seconds on success
    timeout /t 3
    exit
) else (
    :: Clean up the corrupted temporary file if it exists
    if exist "%TEMP_FILE%" del "%TEMP_FILE%"
    echo.
    :: Print error message in Red
    powershell -Command "Write-Host 'Backup failed! (Please check if Unreal Editor is closed)' -ForegroundColor Red"
    echo.
    :: Pause and keep the window open on failure
    pause
    exit
)
