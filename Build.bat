@echo off
REM 设置UTF-8编码
chcp 65001 >nul

REM 获取ESC字符用于ANSI颜色输出
for /F %%a in ('echo prompt $E ^| cmd') do set "ESC=%%a"

REM 初始化路径
set PROJECT_DIR=%~dp0
set UAT_BAT="D:\Epic Games\UnrealEngine-5.8.0-release\Engine\Build\BatchFiles\RunUAT.bat"
set PROJECT_FILE="%PROJECT_DIR%MutateArena.uproject"
set OUTPUT_DIR="%PROJECT_DIR%Build"

REM 选择打包配置 (5秒无操作默认选择Shipping)
echo.
echo [%ESC%[36m1%ESC%[0m] %ESC%[36mDevelopment%ESC%[0m
echo [%ESC%[36m2%ESC%[0m] %ESC%[36mShipping%ESC%[0m
choice /c 12 /n /t 5 /d 2 /m "%ESC%[33mSelect Build Configuration (1 or 2, auto-select 2 in 5s): %ESC%[0m"

if errorlevel 2 (
    set BUILD_CONFIG=Shipping
) else if errorlevel 1 (
    set BUILD_CONFIG=Development
)
echo [INFO] Target Configuration: %ESC%[32m%BUILD_CONFIG%%ESC%[0m
echo.

REM 清理旧的Build文件
set TARGET_CLEAN_DIR="%PROJECT_DIR%Build\Windows"
if exist %TARGET_CLEAN_DIR% (
    rmdir /s /q %TARGET_CLEAN_DIR%
    if exist %TARGET_CLEAN_DIR% (
        echo [WARNING] Failed to clean Windows directory.
    ) else (
        echo [INFO] Cleaned old Windows build.
    )
)

REM 执行UAT打包
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
    -nodebuginfo ^
    -nocompile ^
    -nocompileuat

REM 检查打包结果并播放音效
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

REM 复制README文件
call "%PROJECT_DIR%Misc\README_PLAYER.txt_CopyToBuildFolder.bat"

echo [%ESC%[32m********** SUCCESS **********%ESC%[0m] Build completed!
if exist "%PROJECT_DIR%Misc\CompileSuccess.wav" (
    powershell -c "(New-Object Media.SoundPlayer '%PROJECT_DIR%Misc\CompileSuccess.wav').PlaySync()"
) else (
    powershell -c "[System.Media.SystemSounds]::Asterisk.Play(); Start-Sleep -Milliseconds 500"
)

pause
