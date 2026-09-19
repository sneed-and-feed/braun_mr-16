@echo off
setlocal enabledelayedexpansion
title Deploy BRAUN MR-16 to GitHub Pages (gh-pages)

cd /d "%~dp0"

echo ====================================================================
echo   BRAUN MR-16 - DEPLOY TO GITHUB PAGES (gh-pages branch)
echo ====================================================================
echo.

:: Check if git is installed
where git >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] Git is not installed or not in PATH.
    pause
    exit /b 1
)

:: Verify web showcase exists
if not exist "web\index.html" (
    echo [WARNING] web\index.html not found yet.
    echo Ensuring web directory is present before deployment.
)

echo [1/3] Staging changes in repository...
git add .
git diff --cached --quiet
if %errorlevel% neq 0 (
    for /f "tokens=2 delims==" %%I in ('wmic os get localdatetime /value 2^>nul') do set datetime=%%I
    set timestamp=!datetime:~0,4!-!datetime:~4,2!-!datetime:~6,2! !datetime:~8,2!:!datetime:~10,2!
    git commit -m "deploy: update BRAUN MR-16 build and web assets (!timestamp!)"
)

echo [2/3] Preparing subtree split for web/ directory...
for /f "delims=" %%i in ('git subtree split --prefix web main 2^>nul') do set SPLIT_SHA=%%i

if not defined SPLIT_SHA (
    echo [INFO] Standard subtree split fallback...
    git subtree push --prefix web origin gh-pages
    set DEPLOY_STATUS=!errorlevel!
) else (
    echo [INFO] Pushing subtree commit !SPLIT_SHA! to origin gh-pages...
    git push origin !SPLIT_SHA!:refs/heads/gh-pages --force
    set DEPLOY_STATUS=!errorlevel!
)

echo.
if !DEPLOY_STATUS! equ 0 (
    echo ====================================================================
    echo   DEPLOY SUCCESSFUL!
    echo   BRAUN MR-16 Web Audio Showcase is live at:
    echo   https://sneed-and-feed.github.io/braun_mr-16/
    echo ====================================================================
) else (
    echo [NOTICE] Remote push to gh-pages finished with status code !DEPLOY_STATUS!.
    echo Verify remote repository branch permissions or network connectivity.
)

echo.
pause
endlocal
