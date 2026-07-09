@echo off
setlocal
REM Convenience wrapper so you can run: build.bat
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0build.ps1" %*
exit /b %ERRORLEVEL%
