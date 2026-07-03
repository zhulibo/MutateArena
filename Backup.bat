@echo off
REM Set console to UTF-8 to prevent character encoding issues
chcp 65001 >nul
setlocal

REM Set 7-Zip executable path
set "SEVENZIP=C:\Program Files\7-Zip\7z.exe"
set "SOURCE_DIR=%~dp0"

REM Set temporary directory and temporary zip file path, prevent ZIP file from being deleted if the script fails to run.
set "TEMP_DIR=C:\Users\yoyoy\Desktop\Temp"
set "TEMP_ZIP=%TEMP_DIR%\MutateArena_temp.zip"

REM Set destination directory and final file path
set "DEST_DIR=C:\Users\yoyoy\OneDrive\MutateArena"
set "DEST_FILE=%DEST_DIR%\MutateArena.zip"

REM Check temporary directory
if not exist "%TEMP_DIR%\" (
    powershell -Command "Write-Host 'Error: Temp directory not found! Please create: %TEMP_DIR%' -ForegroundColor Red"
    pause
    exit /b
)

REM Check destination directory
if not exist "%DEST_DIR%\" (
    powershell -Command "Write-Host 'Error: OneDrive destination directory not found! Please check: %DEST_DIR%' -ForegroundColor Red"
    pause
    exit /b
)

REM Set exclusion list using -xr! for recursive exclusion
set "EXCLUDES=-xr!.git -xr!.idea -xr!Binaries -xr!Build -xr!DerivedDataCache -xr!Intermediate -xr!Saved"

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

REM Ensure no leftover temp file exists from a previous failed or interrupted run
if exist "%TEMP_ZIP%" del /f /q "%TEMP_ZIP%"

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
