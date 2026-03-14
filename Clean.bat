@echo off

echo.
echo =================================================================
echo  WARNING: This will permanently delete temporary project files.
echo  (Binaries, DerivedDataCache, Intermediate, Saved, .vs, etc.)
echo =================================================================
echo.
choice /C YN /M "Are you sure you want to proceed?"

IF ERRORLEVEL 2 GOTO :EOF
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

if exist "Plugins" (
    FOR /D /R "Plugins" %%G IN (Binaries) DO (
        IF EXIST "%%G" (
            echo Deleting Plugin Binaries: "%%G"
            RMDIR /S /Q "%%G"
        )
    )

    FOR /D /R "Plugins" %%G IN (Intermediate) DO (
        IF EXIST "%%G" (
            echo Deleting Plugin Intermediate: "%%G"
            RMDIR /S /Q "%%G"
        )
    )
)

echo Cleanup complete!

timeout /t 3
exit
