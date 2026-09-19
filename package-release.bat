@echo off
setlocal
cd /d "%~dp0"
echo ================================================================================
echo BRAUN MR-16 Release Packaging Pipeline
echo Standard: DIN 1451 Technical Specification
echo ================================================================================
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\package-release.ps1" %*
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Packaging pipeline failed with exit code %ERRORLEVEL%.
    exit /b %ERRORLEVEL%
)
echo [INFO] Packaging pipeline completed successfully.
exit /b 0
