@echo off
setlocal

set "SOURCE_PATHS='%~dp0Content'"

set "DEST_FILE='C:\Users\yoyoy\OneDrive\Backup\MutateArena\Content.zip'"

echo Starting backup...
powershell -NoProfile -Command "Compress-Archive -Path %SOURCE_PATHS% -DestinationPath %DEST_FILE% -Force"

if %errorlevel% equ 0 (
    echo  Backup content successful!
) else (
    echo  Backup content failed!
)

echo.
pause
