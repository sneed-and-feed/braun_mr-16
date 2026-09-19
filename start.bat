@echo off
setlocal
title BRAUN MR-16 · Modal Resonator & Kinetic Exciter

cd /d "%~dp0"

echo ====================================================================
echo   BRAUN MR-16 · MODAL RESONATOR & KINETIC EXCITER
echo   "Weniger, aber besser" - Dieter Rams Design Principles
echo ====================================================================
echo.

:: 1. Check for Node.js
where node >nul 2>nul
if %errorlevel% equ 0 (
    echo [OK] Node.js runtime detected.
    echo Starting static audio server on http://localhost:3816 ...
    echo.
    echo Opening browser to http://localhost:3816 ...
    start "" http://localhost:3816
    echo Press Ctrl+C in this terminal window to shut down the server.
    echo --------------------------------------------------------------------
    node server.js
    goto finished
)

:: 2. Fallback check for Python if Node.js is not present
where python >nul 2>nul
if %errorlevel% equ 0 (
    echo [INFO] Node.js not detected, but Python runtime is available.
    echo Launching Python HTTP server on port 3816 ...
    cd web
    start "" http://localhost:3816
    python -m http.server 3816
    goto finished
)

:: 3. Neither runtime found
echo [ERROR] Neither Node.js nor Python was found on your system PATH.
echo.
echo To run this modal resonator showcase locally, please install Node.js:
echo    https://nodejs.org/
echo.
pause
exit /b 1

:finished
if %errorlevel% neq 0 (
    echo.
    echo Server process terminated with exit code %errorlevel%.
    pause
)
endlocal
