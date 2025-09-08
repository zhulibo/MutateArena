@echo off
setlocal

set baseDir=%~dp0

set source=%baseDir%README.txt
set target=%baseDir%Build\Windows\

copy "%source%" "%target%"

pause
