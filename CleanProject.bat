@echo off

echo Starting cleanup...

if exist "Binaries" (
    echo Deleting Binaries...
    RMDIR /S /Q "Binaries"
)
if exist "DerivedDataCache" (
    echo Deleting DerivedDataCache...
    RMDIR /S /Q "DerivedDataCache"
)
if exist "Intermediate" (
    echo Deleting Intermediate...
    RMDIR /S /Q "Intermediate"
)
if exist "Saved" (
    echo Deleting Saved...
    RMDIR /S /Q "Saved"
)

if exist ".vs" (
    echo Deleting .vs...
    RMDIR /S /Q ".vs"
)
if exist ".vsconfig" (
    echo Deleting .vsconfig...
    DEL /F /Q ".vsconfig"
)
if exist "MutateArena.sln" (
    echo Deleting MutateArena.sln...
    DEL /F /Q "MutateArena.sln"
)

echo Cleanup complete!

pause
