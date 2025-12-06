@echo off
setlocal EnableDelayedExpansion

:: --- 配置区域 ---
set "SOURCE_PATHS='%~dp0Config','%~dp0Content','%~dp0Plugins','%~dp0Source','%~dp0.gitignore','%~dp0BackupContent.bat','%~dp0BackupProject.bat','%~dp0CleanProject.bat','%~dp0LICENSE','%~dp0MutateArena.uproject','%~dp0README.md','%~dp0README.txt','%~dp0README.txt_CopyToBuildFolder.bat'"

set "BACKUP_DIR=D:\Backup"
set "BASE_NAME=MutateArena"
set "EXT=.zip"
:: ----------------

echo Searching for the latest backup version...

:: 初始化最大编号为 -1 (代表没有找到任何文件)
set /a "max_num=-1"

:: 遍历所有匹配 MutateArena*.zip 的文件
for %%F in ("%BACKUP_DIR%\%BASE_NAME%*%EXT%") do (
    set "filename=%%~nF"
    
    :: 将文件名中的 "MutateArena" 替换为空，只剩下数字部分 (例如 MutateArena99 -> 99)
    set "num_part=!filename:%BASE_NAME%=!"

    :: 情况1: 如果替换后为空，说明文件是 MutateArena.zip (即第0号)
    if "!num_part!"=="" (
        set /a "current_num=0"
    ) else (
        :: 情况2: 尝试读取数字 (例如 99)
        set /a "current_num=!num_part!" 2>nul
    )

    :: 如果当前文件的数字 比 记录的最大值 大，则更新最大值
    if !current_num! gtr !max_num! (
        set /a "max_num=!current_num!"
    )
)

:: 计算下一个文件的路径
if !max_num! equ -1 (
    :: 如果文件夹是空的，就叫 MutateArena.zip
    set "TARGET_PATH=%BACKUP_DIR%\%BASE_NAME%%EXT%"
) else (
    :: 如果找到了文件 (比如最大是 99)，下一个就是 100
    set /a "next_num=max_num + 1"
    set "TARGET_PATH=%BACKUP_DIR%\%BASE_NAME%!next_num!%EXT%"
)

:: 加上单引号适配 PowerShell
set "DEST_FILE='%TARGET_PATH%'"

echo Target file will be: %DEST_FILE%
echo Starting backup...

:: 执行压缩
powershell -NoProfile -Command "Compress-Archive -Path %SOURCE_PATHS% -DestinationPath %DEST_FILE% -Force"

if %errorlevel% equ 0 (
    echo  Backup project successful: %DEST_FILE%
) else (
    echo  Backup project failed!
)

pause
