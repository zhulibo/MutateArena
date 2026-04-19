@echo off
REM Set console to UTF-8 to prevent character encoding issues
chcp 65001 >nul

REM Get the directory of the current batch file (Project root, includes trailing backslash \)
set PROJECT_DIR=%~dp0

REM Set Unreal Automation Tool (UAT) path
set UAT_BAT="D:\Epic Games\UnrealEngine-release\Engine\Build\BatchFiles\RunUAT.bat"

REM Build dynamic paths for the project file and output directory
set PROJECT_FILE="%PROJECT_DIR%MutateArena.uproject"
set OUTPUT_DIR="%PROJECT_DIR%Build"

echo Starting Shipping build for MutateArena...
echo Project File: %PROJECT_FILE%
echo Output Directory: %OUTPUT_DIR%
echo.

REM Call UAT for automated packaging
call %UAT_BAT% BuildCookRun ^
    -project=%PROJECT_FILE% ^
    -target=MutateArena ^
    -platform=Win64 ^
    -clientconfig=Shipping ^
    -nop4 ^
    -utf8output ^
    -nocompileeditor ^
    -skipbuildeditor ^
    -cook ^
    -build ^
    -stage ^
    -archive ^
    -package ^
    -archivedirectory=%OUTPUT_DIR% ^
    -prereqs ^
    -applocaldirectory="$(EngineDir)/Binaries/ThirdParty/AppLocalDependencies" ^
    -CrashReporter ^
    -nodebuginfo ^
    -nocompile ^
    -nocompileuat

REM Check if the build was successful
if %errorlevel% neq 0 (
    echo.
    echo [ERROR] UAT build process failed. Please check the log above!
    pause
    exit /b %errorlevel%
)

echo.
echo [SUCCESS] Game packaged successfully to: %OUTPUT_DIR%
echo Running post-build script to copy README...

REM Call the copy script located in the project directory
call "%PROJECT_DIR%Misc\README_PLAYER.txt_CopyToBuildFolder.bat"

echo.
echo All automated processes completed successfully!
pause
