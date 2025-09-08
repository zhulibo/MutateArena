@echo off
setlocal

set "SOURCE_PATHS='%~dp0Config','%~dp0Content','%~dp0Plugins','%~dp0Source','%~dp0.gitignore','%~dp0BackupProject.bat','%~dp0BackupContent.bat','%~dp0CleanProject.bat','%~dp0LICENSE','%~dp0MutateArena.uproject','%~dp0README.md','%~dp0README.txt','%~dp0README.txt_CopyToBuildFolder.bat'"

set "DEST_FILE='C:\Users\yoyoy\OneDrive\Backup\MutateArena\MutateArena.zip'"

echo Starting backup...
powershell -NoProfile -Command "Compress-Archive -Path %SOURCE_PATHS% -DestinationPath %DEST_FILE% -Force"

if %errorlevel% equ 0 (
    echo  Backup project successful!
) else (
    echo  Backup project failed!
)

pause
