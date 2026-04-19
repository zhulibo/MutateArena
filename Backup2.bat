@echo off
REM Set console to UTF-8 to prevent character encoding issues
chcp 65001 >nul
setlocal

REM Set 7-Zip executable path
set "SEVENZIP=C:\Program Files\7-Zip\7z.exe"
set "SOURCE_DIR=%~dp0"
set "TEMP_ZIP=C:\Users\yoyoy\Desktop\Temp\MutateArena_temp.zip"
set "DEST_FILE=C:\Users\yoyoy\OneDrive\Backup\MutateArena\MutateArena.zip"

REM Set exclusion list using -xr! for recursive exclusion
set "EXCLUDES=-xr!UltraDynamicSky -xr!.git -xr!.idea -xr!Binaries -xr!Build -xr!DerivedDataCache -xr!Intermediate -xr!Saved"

REM Check if 7-Zip exists at the specified path
if not exist "%SEVENZIP%" (
    powershell -Command "Write-Host 'Error: 7-Zip not found! Please check the path: %SEVENZIP%' -ForegroundColor Red"
    pause
    exit /b
)

echo.
set "DISPLAY_LIST=%EXCLUDES:-xr!=%"
powershell -Command "Write-Host 'Skip %DISPLAY_LIST%' -ForegroundColor Cyan"
echo.

echo Starting ultra-fast backup using 7-Zip...

REM Execute 7-Zip compression
REM a: Add to archive
REM -tzip: Format as zip
REM -mx5: Compression level 5 (Standard)
REM -ssw: Compress files open for writing
"%SEVENZIP%" a -tzip "%TEMP_ZIP%" "%SOURCE_DIR%*" %EXCLUDES% -mx5 -ssw

REM Check the exit code of the 7-Zip command
if %errorlevel% equ 0 (
    REM Replace the old backup only if the compression succeeded
    move /y "%TEMP_ZIP%" "%DEST_FILE%" >nul
    echo.
    
    REM Print success message in Green
    powershell -Command "Write-Host 'Backup successful!' -ForegroundColor Green"
    echo.
    
    REM Close automatically after 5 seconds on success
    timeout /t 5
    exit /b
) else (
    REM Clean up the corrupted temporary file if it exists
    if exist "%TEMP_ZIP%" del "%TEMP_ZIP%"
    echo.
    
    REM Print error message in Red
    powershell -Command "Write-Host 'Backup failed! (Error code: %errorlevel%)' -ForegroundColor Red"
    echo.
    
    REM Pause and keep the window open on failure
    pause
    exit /b %errorlevel%
)
