@echo off
setlocal

set baseDir=%~dp0

set source=%baseDir%README_PLAYER.txt

set target=%baseDir%..\Build\Windows\

REM 复制文件
copy "%source%" "%target%"

echo Copy README_PLAYER.txt complete!

timeout /t 3
exit /b
