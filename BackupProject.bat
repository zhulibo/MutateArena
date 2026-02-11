@echo off

setlocal

set "SOURCE_DIR=%~dp0"

set "DEST_FILE=C:\Users\yoyoy\OneDrive\Backup\MutateArena\MutateArena.zip"

set "EXCLUDE_ITEMS='.git', '.idea', 'Binaries', 'Build', 'DerivedDataCache', 'Intermediate', 'Saved'"

echo Starting backup...

powershell -NoProfile -Command "Get-ChildItem -Path '%SOURCE_DIR%' -Exclude %EXCLUDE_ITEMS% | Compress-Archive -DestinationPath '%DEST_FILE%' -Force"

if %errorlevel% equ 0 (
    echo  Backup content successful!
) else (
    echo  Backup content failed!
)

echo.

timeout /t 3
exit
