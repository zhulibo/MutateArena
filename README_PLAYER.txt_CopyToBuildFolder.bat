@echo off
setlocal

set baseDir=%~dp0

set source=%baseDir%README_PLAYER.txt
set target=%baseDir%Build\Windows\

copy "%source%" "%target%"

echo Copy complete!

timeout /t 3
exit
