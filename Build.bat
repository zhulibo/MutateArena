@echo off
REM Set UTF-8 encoding
chcp 65001 >nul

REM Get ESC character for ANSI color output
for /F %%a in ('echo prompt $E ^| cmd') do set "ESC=%%a"

REM Initialize paths
set PROJECT_DIR=%~dp0
set UAT_BAT="D:\Epic Games\UnrealEngine-5.8.0-release\Engine\Build\BatchFiles\RunUAT.bat"
set PROJECT_FILE="%PROJECT_DIR%MutateArena.uproject"
set OUTPUT_DIR="%PROJECT_DIR%Build"
set GAME_INI="%PROJECT_DIR%Config\DefaultGame.ini"

REM Extract current version from DefaultGame.ini using PowerShell
set CURRENT_VERSION=1.0.0
if exist %GAME_INI% (
    for /f "delims=" %%I in ('powershell -NoProfile -Command "(Get-Content '%GAME_INI%') -match '^ProjectVersion=' | ForEach-Object { ($_.Split('='))[1] }"') do set "CURRENT_VERSION=%%I"
)

REM Prompt user for version change (Press ENTER to keep current)
echo.
echo [INFO] Current Project Version: %ESC%[36m%CURRENT_VERSION%%ESC%[0m
set "NEW_VERSION="
set /p NEW_VERSION="%ESC%[33mEnter new version (Press ENTER to keep current): %ESC%[0m"

REM Update version in DefaultGame.ini if a new one was provided
if not "%NEW_VERSION%"=="" (
    if not "%NEW_VERSION%"=="%CURRENT_VERSION%" (
        echo [INFO] Updating version to: %ESC%[32m%NEW_VERSION%%ESC%[0m
        if exist %GAME_INI% (
            powershell -NoProfile -Command "(Get-Content '%GAME_INI%') -replace '^ProjectVersion=.*', 'ProjectVersion=%NEW_VERSION%' | Set-Content '%GAME_INI%'"
        ) else (
            echo [%ESC%[31mWARNING%ESC%[0m] DefaultGame.ini not found. Cannot update version.
        )
    ) else (
        echo [INFO] Version remains unchanged: %ESC%[32m%CURRENT_VERSION%%ESC%[0m
    )
) else (
    echo [INFO] Version remains unchanged: %ESC%[32m%CURRENT_VERSION%%ESC%[0m
)

REM Select build configuration (Press ENTER to default to Shipping)
echo.
echo [%ESC%[36m1%ESC%[0m] %ESC%[36mDevelopment%ESC%[0m
echo [%ESC%[36m2%ESC%[0m] %ESC%[36mShipping%ESC%[0m
set "CONFIG_CHOICE="
set /p CONFIG_CHOICE="%ESC%[33mSelect Build Configuration (1 for Dev, 2 for Shipping) [Press ENTER for Shipping]: %ESC%[0m"

REM Evaluate user input (Default to Shipping if empty or invalid, unless exactly '1')
if "%CONFIG_CHOICE%"=="1" (
    set BUILD_CONFIG=Development
) else (
    set BUILD_CONFIG=Shipping
)
echo [INFO] Target Configuration: %ESC%[32m%BUILD_CONFIG%%ESC%[0m
echo.

REM Clean previous Windows build directory
set TARGET_CLEAN_DIR="%PROJECT_DIR%Build\Windows"
if exist %TARGET_CLEAN_DIR% (
    rmdir /s /q %TARGET_CLEAN_DIR%
    if exist %TARGET_CLEAN_DIR% (
        echo [%ESC%[31mWARNING%ESC%[0m] Failed to clean Windows directory.
    ) else (
        echo [INFO] Cleaned old Windows build.
    )
)

REM Execute UAT build process
call %UAT_BAT% BuildCookRun ^
    -project=%PROJECT_FILE% ^
    -target=MutateArena ^
    -platform=Win64 ^
    -clientconfig=%BUILD_CONFIG% ^
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
    -nocompile ^
    -nocompileuat

REM Check build result and play sound notification
if %errorlevel% neq 0 (
    echo [%ESC%[31m********** ERROR **********%ESC%[0m] Build failed!
    if exist "%PROJECT_DIR%Misc\CompileFailed.wav" (
        powershell -c "(New-Object Media.SoundPlayer '%PROJECT_DIR%Misc\CompileFailed.wav').PlaySync()"
    ) else (
        powershell -c "[System.Media.SystemSounds]::Hand.Play(); Start-Sleep -Milliseconds 500"
    )
    pause
    exit /b %errorlevel%
)

REM Copy README file to build folder
call "%PROJECT_DIR%Misc\README_PLAYER.txt_CopyToBuildFolder.bat"

echo [%ESC%[32m********** SUCCESS **********%ESC%[0m] Build completed!
if exist "%PROJECT_DIR%Misc\CompileSuccess.wav" (
    powershell -c "(New-Object Media.SoundPlayer '%PROJECT_DIR%Misc\CompileSuccess.wav').PlaySync()"
) else (
    powershell -c "[System.Media.SystemSounds]::Asterisk.Play(); Start-Sleep -Milliseconds 500"
)

pause