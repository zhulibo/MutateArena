@echo off

echo.
echo =================================================================
echo  WARNING: This will permanently delete temporary project files.
echo  (Binaries, DerivedDataCache, Intermediate, Saved, .vs, etc.)
echo =================================================================
echo.
choice /C YN /M "Are you sure you want to proceed?"

REM If user chooses 'N' (second option), exit the script.
IF ERRORLEVEL 2 GOTO :EOF

REM If user chooses 'Y' (first option), continue to the cleaning part.
IF ERRORLEVEL 1 GOTO :START_CLEAN


:START_CLEAN
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
